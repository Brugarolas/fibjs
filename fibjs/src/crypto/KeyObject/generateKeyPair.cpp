/*
 * generateKeyPair.cpp
 *
 *  Created on: feb 24, 2024
 *      Author: lion
 */

#include "object.h"
#include "crypto_util.h"
#include "ifs/crypto.h"
#include "Buffer.h"
#include "KeyObject.h"

namespace fibjs {

static EVP_PKEY_CTX* init_param_ctx(int nid)
{
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(nid, nullptr);
    if (!ctx)
        return nullptr;

    if (EVP_PKEY_paramgen_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        return nullptr;
    }

    return ctx;
}

static EVP_PKEY* genkey_from_param(EVP_PKEY_CTX* param_ctx)
{
    EVP_PKEY* raw_params = nullptr;
    if (EVP_PKEY_paramgen(param_ctx, &raw_params) <= 0)
        return nullptr;

    EVPKeyPointer key_param = raw_params;
    EVPKeyCtxPointer key_ctx = EVP_PKEY_CTX_new(raw_params, nullptr);
    if (!key_ctx)
        return nullptr;

    if (EVP_PKEY_keygen_init(key_ctx) <= 0)
        return nullptr;

    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(key_ctx, &pkey) <= 0)
        return nullptr;

    return pkey;
}

result_t KeyObject::generateRsaKey(int nid, generateKeyPairParam* param)
{
    EVPKeyCtxPointer key_ctx = EVP_PKEY_CTX_new_id(nid, nullptr);

    if (EVP_PKEY_keygen_init(key_ctx) <= 0)
        return openssl_error();

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(key_ctx, param->modulusLength) <= 0)
        return openssl_error();

    if (param->publicExponent != 0x10001) {
        BignumPointer e = BN_new();

        if (BN_set_word(e, param->publicExponent) != 1)
            return openssl_error();

        if (EVP_PKEY_CTX_set_rsa_keygen_pubexp(key_ctx, e) <= 0)
            return openssl_error();

        e.release();
    }

    if (nid == EVP_PKEY_RSA_PSS) {
        const EVP_MD* md = nullptr;

        if (!param->hashAlgorithm.empty()) {
            md = _evp_md_type(param->hashAlgorithm.c_str());
            if (!md)
                return Runtime::setError("Invalid hashAlgorithm");

            if (EVP_PKEY_CTX_set_rsa_pss_keygen_md(key_ctx, md) <= 0)
                return openssl_error();
        }

        if (!param->mgf1Algorithm.empty()) {
            const EVP_MD* md = _evp_md_type(param->mgf1Algorithm.c_str());
            if (!md)
                return Runtime::setError("Invalid mgf1Algorithm");

            if (EVP_PKEY_CTX_set_rsa_pss_keygen_mgf1_md(key_ctx, md) <= 0)
                return openssl_error();
        }

        if (param->saltLength <= 0 && md)
            param->saltLength = EVP_MD_size(md);

        if (param->saltLength > 0) {
            if (EVP_PKEY_CTX_set_rsa_pss_keygen_saltlen(key_ctx, param->saltLength) <= 0)
                return openssl_error();
        }
    }

    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(key_ctx, &pkey) <= 0)
        return openssl_error();
    m_pkey = pkey;

    m_keyType = kKeyTypePrivate;

    return 0;
}

result_t KeyObject::generateDsaKey(int nid, generateKeyPairParam* param)
{
    EVPKeyCtxPointer param_ctx = init_param_ctx(nid);
    if (!param_ctx)
        return openssl_error();

    if (EVP_PKEY_CTX_set_dsa_paramgen_bits(param_ctx, param->modulusLength) <= 0)
        return openssl_error();

    if (param->divisorLength > 0) {
        if (EVP_PKEY_CTX_set_dsa_paramgen_q_bits(param_ctx, param->divisorLength) <= 0)
            return openssl_error();
    }

    m_pkey = genkey_from_param(param_ctx);
    if (!m_pkey)
        return openssl_error();

    m_keyType = kKeyTypePrivate;

    return 0;
}

result_t KeyObject::generateEcKey(int nid, generateKeyPairParam* param)
{
    EVPKeyCtxPointer param_ctx = init_param_ctx(nid);
    if (!param_ctx)
        return openssl_error();

    int cid;
    if (nid == EVP_PKEY_EC) {
        cid = GetCurveFromName(param->namedCurve.c_str());
        if (cid == NID_undef)
            return Runtime::setError("Invalid curve name");
    } else
        cid = NID_sm2;

    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(param_ctx, cid) <= 0)
        return openssl_error();

    if (param->paramEncoding.empty() || param->paramEncoding == "named") {
        if (EVP_PKEY_CTX_set_ec_param_enc(param_ctx, OPENSSL_EC_NAMED_CURVE) <= 0)
            return openssl_error();
    } else if (param->paramEncoding == "explicit") {
        if (EVP_PKEY_CTX_set_ec_param_enc(param_ctx, OPENSSL_EC_EXPLICIT_CURVE) <= 0)
            return openssl_error();
    } else {
        return Runtime::setError("Invalid paramEncoding");
    }

    m_pkey = genkey_from_param(param_ctx);
    if (!m_pkey)
        return openssl_error();

    m_keyType = kKeyTypePrivate;

    return 0;
}

result_t KeyObject::generateEdKey(int nid, generateKeyPairParam* param)
{
    EVPKeyCtxPointer key_ctx = EVP_PKEY_CTX_new_id(nid, nullptr);
    if (!key_ctx)
        return openssl_error();

    if (EVP_PKEY_keygen_init(key_ctx) <= 0)
        return openssl_error();

    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(key_ctx, &pkey) <= 0)
        return openssl_error();
    m_pkey = pkey;

    m_keyType = kKeyTypePrivate;

    return 0;
}

result_t KeyObject::generateKey(exlib::string type, generateKeyPairParam* param)
{
    if (type == "rsa")
        return generateRsaKey(EVP_PKEY_RSA, param);
    else if (type == "rsa-pss")
        return generateRsaKey(EVP_PKEY_RSA_PSS, param);
    else if (type == "dsa")
        return generateDsaKey(EVP_PKEY_DSA, param);
    else if (type == "ec")
        return generateEcKey(EVP_PKEY_EC, param);
    else if (type == "sm2")
        return generateEcKey(EVP_PKEY_SM2, param);
    else if (type == "ed25519")
        return generateEdKey(EVP_PKEY_ED25519, param);
    else if (type == "ed448")
        return generateEdKey(EVP_PKEY_ED448, param);
    else if (type == "x25519")
        return generateEdKey(EVP_PKEY_X25519, param);
    else if (type == "x448")
        return generateEdKey(EVP_PKEY_X448, param);

    return Runtime::setError("Invalid key type");
}

#define GET_GEN_PARAM(name)                                          \
    hr = GetConfigValue(isolate, options, #name, param->name, true); \
    if (hr < 0 && hr != CALL_E_PARAMNOTOPTIONAL)                     \
        return hr;

result_t crypto_base::generateKeyPair(exlib::string type, v8::Local<v8::Object> options,
    obj_ptr<GenerateKeyPairType>& retVal, AsyncEvent* ac)
{
    if (ac->isSync()) {
        Isolate* isolate = ac->isolate();
        result_t hr;

        obj_ptr<generateKeyPairParam> param = new generateKeyPairParam();

        GET_GEN_PARAM(modulusLength);
        GET_GEN_PARAM(publicExponent);
        GET_GEN_PARAM(hashAlgorithm);
        GET_GEN_PARAM(mgf1Algorithm);
        GET_GEN_PARAM(saltLength);
        GET_GEN_PARAM(divisorLength);
        GET_GEN_PARAM(namedCurve);
        GET_GEN_PARAM(paramEncoding);

        ac->m_ctx.resize(1);
        ac->m_ctx[0] = param;

        return CALL_E_NOSYNC;
    } else if (ac->isPost()) {
        Isolate* isolate = ac->isolate();
        obj_ptr<generateKeyPairParam> param = (generateKeyPairParam*)ac->m_ctx[0].object();

        obj_ptr<KeyObject_base> key;
        v8::Local<v8::Value> v;
        result_t hr;

        v8::Local<v8::Object> publicKeyEncoding;
        hr = GetConfigValue(isolate, options, "publicKeyEncoding", publicKeyEncoding);
        if (hr == 0) {
            key = (KeyObject_base*)retVal->publicKey.object();
            hr = key->_export(publicKeyEncoding, v);
            if (hr != 0)
                return hr;

            retVal->publicKey = v;
        } else if (hr != CALL_E_PARAMNOTOPTIONAL)
            return hr;

        v8::Local<v8::Object> privateKeyEncoding;
        hr = GetConfigValue(isolate, options, "privateKeyEncoding", privateKeyEncoding);
        if (hr == 0) {
            key = (KeyObject_base*)retVal->privateKey.object();
            hr = key->_export(privateKeyEncoding, v);
            if (hr != 0)
                return hr;

            retVal->privateKey = v;
        } else if (hr != CALL_E_PARAMNOTOPTIONAL)
            return hr;

        return 0;
    }

    obj_ptr<generateKeyPairParam> param = (generateKeyPairParam*)ac->m_ctx[0].object();

    retVal = new GenerateKeyPairType();

    obj_ptr<KeyObject> privaterKey = new KeyObject();
    result_t hr = privaterKey->generateKey(type, param);
    if (hr < 0)
        return hr;

    obj_ptr<KeyObject_base> publicKey;
    hr = createPublicKey(privaterKey, publicKey);
    if (hr < 0)
        return hr;

    retVal->privateKey = privaterKey;
    retVal->publicKey = publicKey;

    ac->setPost();

    return 0;
}

}
