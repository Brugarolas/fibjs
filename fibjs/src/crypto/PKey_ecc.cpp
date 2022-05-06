/*
 * PKey_ecc.cpp
 *
 *  Created on: May 2, 2022
 *      Author: lion
 */

#include "Buffer.h"
#include "PKey_impl.h"
#include "ssl.h"

extern "C" {
int ecsdsa_sign(mbedtls_ecp_keypair* ctx, int sdsa, mbedtls_ecp_keypair* to_ctx, const unsigned char* hash, size_t hlen,
    unsigned char* sig, size_t* slen, int (*f_rng)(void*, unsigned char*, size_t), void* p_rng);
int ecsdsa_verify(mbedtls_ecp_keypair* ctx, int sdsa, mbedtls_ecp_keypair* to_ctx, const unsigned char* hash, size_t hlen,
    const unsigned char* sig, size_t slen, int (*f_rng)(void*, unsigned char*, size_t), void* p_rng);
}

namespace fibjs {

struct curve_info {
    int32_t id;
    const char* name;
};

static const curve_info supported_curves[] = {
    { MBEDTLS_ECP_DP_SECP192R1, "P-192" },
    { MBEDTLS_ECP_DP_SECP192R1, "secp192r1" },
    { MBEDTLS_ECP_DP_SECP224R1, "P-224" },
    { MBEDTLS_ECP_DP_SECP224R1, "secp224r1" },
    { MBEDTLS_ECP_DP_SECP256R1, "P-256" },
    { MBEDTLS_ECP_DP_SECP256R1, "secp256r1" },
    { MBEDTLS_ECP_DP_SECP384R1, "P-384" },
    { MBEDTLS_ECP_DP_SECP384R1, "secp384r1" },
    { MBEDTLS_ECP_DP_SECP521R1, "P-521" },
    { MBEDTLS_ECP_DP_SECP521R1, "secp521r1" },
    { MBEDTLS_ECP_DP_BP256R1, "brainpoolP256r1" },
    { MBEDTLS_ECP_DP_BP384R1, "brainpoolP384r1" },
    { MBEDTLS_ECP_DP_BP512R1, "brainpoolP512r1" },
    { MBEDTLS_ECP_DP_SECP192K1, "secp192k1" },
    { MBEDTLS_ECP_DP_SECP224K1, "secp224k1" },
    { MBEDTLS_ECP_DP_SECP256K1, "P-256K" },
    { MBEDTLS_ECP_DP_SECP256K1, "secp256k1" },
    { MBEDTLS_ECP_DP_SM2P256R1, "SM2" },
    { MBEDTLS_ECP_DP_SM2P256R1, "sm2" },
    { MBEDTLS_ECP_DP_SM2P256R1, "sm2p256r1" },
    { MBEDTLS_ECP_DP_ED25519, "Ed25519" },
    { MBEDTLS_ECP_DP_ED25519, "ed25519" },
    { MBEDTLS_ECP_DP_BLS12381_G1, "BLS12381_G1" },
    { MBEDTLS_ECP_DP_BLS12381_G2, "BLS12381_G2" },
};

int32_t PKey_ecc::get_curve_id(exlib::string& curve)
{
    int32_t i;

    for (i = 0; i < ARRAYSIZE(supported_curves); i++)
        if (!strcmp(curve.c_str(), supported_curves[i].name))
            return supported_curves[i].id;

    return MBEDTLS_ECP_DP_NONE;
}

const char* PKey_ecc::get_curve_name(int32_t id)
{
    int32_t i;

    for (i = 0; i < ARRAYSIZE(supported_curves); i++)
        if (supported_curves[i].id == id)
            return supported_curves[i].name;

    return NULL;
}

static int ecp_group_load(mbedtls_ecp_group* grp, int32_t id)
{
    if (id >= MBEDTLS_ECP_DP_ED25519) {
        grp->id = (mbedtls_ecp_group_id)id;
        return 0;
    }

    return mbedtls_ecp_group_load(grp, (mbedtls_ecp_group_id)id);
}

PKey_ecc::PKey_ecc()
{
    mbedtls_pk_setup(&m_key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
}

PKey_ecc::PKey_ecc(mbedtls_pk_context& key)
    : PKey(key)
{
    mbedtls_ecp_keypair* ecp = mbedtls_pk_ec(m_key);
    int32_t id = ecp->grp.id;

    if (id < MBEDTLS_ECP_DP_ED25519) {
        m_alg = id == MBEDTLS_ECP_DP_SM2P256R1 ? "SM2" : "ECDSA";
        if (mbedtls_mpi_cmp_int(&ecp->d, 0) && !mbedtls_mpi_cmp_int(&ecp->Q.X, 0))
            mbedtls_ecp_mul(&ecp->grp, &ecp->Q, &ecp->d, &ecp->grp.G,
                mbedtls_ctr_drbg_random, &g_ssl.ctr_drbg);
    }
}

PKey_ecc::PKey_ecc(int32_t id)
{
    mbedtls_pk_type_t pk_type;

    if (id == MBEDTLS_ECP_DP_SM2P256R1) {
        pk_type = MBEDTLS_PK_SM2;
        m_alg = "SM2";
    } else {
        pk_type = MBEDTLS_PK_ECKEY;
        m_alg = "ECDSA";
    }

    mbedtls_pk_setup(&m_key, mbedtls_pk_info_from_type(pk_type));
    mbedtls_ecp_gen_key((mbedtls_ecp_group_id)id, mbedtls_pk_ec(m_key),
        mbedtls_ctr_drbg_random, &g_ssl.ctr_drbg);
}

PKey_ecc* PKey_ecc::create(mbedtls_pk_context& key)
{
    int32_t id = mbedtls_pk_ec(key)->grp.id;

    if (id == MBEDTLS_ECP_DP_ED25519)
        return new PKey_25519(key);
    else if (id == MBEDTLS_ECP_DP_BLS12381_G1)
        return new PKey_bls_g1(key);
    else if (id == MBEDTLS_ECP_DP_BLS12381_G2)
        return new PKey_bls_g2(key);

    return new PKey_ecc(key);
}

result_t PKey_ecc::generateKey(exlib::string curve, obj_ptr<PKey_base>& retVal)
{
    int32_t id = get_curve_id(curve);
    switch (id) {
    case MBEDTLS_ECP_DP_NONE:
        return CHECK_ERROR(Runtime::setError("PKey: Unknown curve"));
    case MBEDTLS_ECP_DP_ED25519:
        retVal = new PKey_25519();
        break;
    case MBEDTLS_ECP_DP_BLS12381_G1:
        retVal = new PKey_bls_g1();
        break;
    case MBEDTLS_ECP_DP_BLS12381_G2:
        retVal = new PKey_bls_g2();
        break;
    default:
        retVal = new PKey_ecc(id);
    }

    return 0;
}

result_t PKey_ecc::get_publicKey(obj_ptr<PKey_base>& retVal)
{
    bool priv;

    result_t hr = isPrivate(priv);
    if (hr < 0)
        return hr;

    if (!priv)
        return CALL_RETURN_NULL;

    mbedtls_pk_context ctx;

    mbedtls_pk_init(&ctx);
    mbedtls_pk_setup(&ctx, m_key.pk_info);

    mbedtls_ecp_keypair* ecp = mbedtls_pk_ec(m_key);
    mbedtls_ecp_keypair* ecp1 = mbedtls_pk_ec(ctx);

    ecp_group_load(&ecp1->grp, ecp->grp.id);
    mbedtls_ecp_copy(&ecp1->Q, &ecp->Q);

    obj_ptr<PKey_ecc> pk = PKey_ecc::create(ctx);
    pk->m_alg = m_alg;

    retVal = pk;

    return 0;
}

result_t PKey_ecc::isPrivate(bool& retVal)
{
    mbedtls_ecp_keypair* ecp = mbedtls_pk_ec(m_key);
    retVal = mbedtls_mpi_cmp_int(&ecp->d, 0);
    return 0;
}

result_t PKey_ecc::clone(obj_ptr<PKey_base>& retVal)
{
    mbedtls_pk_context ctx;

    mbedtls_pk_init(&ctx);
    mbedtls_pk_setup(&ctx, m_key.pk_info);

    mbedtls_ecp_keypair* ecp = mbedtls_pk_ec(m_key);
    mbedtls_ecp_keypair* ecp1 = mbedtls_pk_ec(ctx);

    ecp_group_load(&ecp1->grp, ecp->grp.id);
    mbedtls_mpi_copy(&ecp1->d, &ecp->d);
    mbedtls_ecp_copy(&ecp1->Q, &ecp->Q);

    obj_ptr<PKey_ecc> pk = PKey_ecc::create(ctx);
    pk->m_alg = m_alg;

    retVal = pk;

    return 0;
}

result_t PKey_ecc::equal(PKey_base* key, bool& retVal)
{
    retVal = false;

    obj_ptr<PKey> pkey = (PKey*)key;

    mbedtls_pk_type_t type = mbedtls_pk_get_type(&m_key);
    mbedtls_pk_type_t type1 = mbedtls_pk_get_type(&pkey->m_key);
    if (type != type1)
        return 0;

    mbedtls_ecp_keypair* ecp = mbedtls_pk_ec(m_key);
    mbedtls_ecp_keypair* ecp1 = mbedtls_pk_ec(pkey->m_key);

    if (ecp->grp.id != ecp1->grp.id)
        return 0;

    if (mbedtls_mpi_cmp_mpi(&ecp->Q.X, &ecp1->Q.X)
        || mbedtls_mpi_cmp_mpi(&ecp->Q.Y, &ecp1->Q.Y)
        || mbedtls_mpi_cmp_mpi(&ecp->d, &ecp1->d))
        return 0;

    retVal = true;
    return 0;
}

result_t PKey_ecc::sign(Buffer_base* data, int32_t alg, obj_ptr<Buffer_base>& retVal, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    if (m_alg == "ECSDSA")
        return sign(data, (PKey_base*)NULL, retVal, ac);

    return PKey::sign(data, alg, retVal, ac);
}

result_t PKey_ecc::sign(Buffer_base* data, PKey_base* key, obj_ptr<Buffer_base>& retVal, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    if (m_alg != "ECSDSA" && m_alg != "SM2")
        return CHECK_ERROR(CALL_E_INVALID_CALL);

    result_t hr;
    bool priv;

    hr = isPrivate(priv);
    if (hr < 0)
        return hr;

    if (!priv)
        return CHECK_ERROR(CALL_E_INVALID_CALL);

    obj_ptr<PKey> to_key = (PKey*)key;
    if (to_key) {
        mbedtls_pk_type_t type;

        type = mbedtls_pk_get_type(&to_key->m_key);
        if (type != MBEDTLS_PK_ECKEY && type != MBEDTLS_PK_SM2)
            return CHECK_ERROR(CALL_E_INVALIDARG);

        mbedtls_ecp_keypair* ecp1 = mbedtls_pk_ec(m_key);
        mbedtls_ecp_keypair* ecp2 = mbedtls_pk_ec(to_key->m_key);
        if (ecp1->grp.id != ecp2->grp.id)
            return CHECK_ERROR(Runtime::setError("Public key is not valid for specified curve"));
    }

    int32_t ret;
    exlib::string str;
    exlib::string output;
    size_t olen = MBEDTLS_ECDSA_MAX_LEN;

    data->toString(str);
    output.resize(MBEDTLS_ECDSA_MAX_LEN);

    ret = ecsdsa_sign(mbedtls_pk_ec(m_key), m_alg == "ECSDSA", key ? mbedtls_pk_ec(to_key->m_key) : NULL,
        (const unsigned char*)str.c_str(), str.length(), (unsigned char*)output.c_buffer(), &olen,
        mbedtls_ctr_drbg_random, &g_ssl.ctr_drbg);
    if (ret != 0)
        return CHECK_ERROR(_ssl::setError(ret));

    output.resize(olen);
    retVal = new Buffer(output);

    return 0;
}

result_t PKey_ecc::verify(Buffer_base* data, Buffer_base* sign, int32_t alg, bool& retVal, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    if (m_alg == "ECSDSA")
        return verify(data, sign, (PKey_base*)NULL, retVal, ac);

    return PKey::verify(data, sign, alg, retVal, ac);
}

result_t PKey_ecc::verify(Buffer_base* data, Buffer_base* sign, PKey_base* key, bool& retVal, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    if (m_alg != "ECSDSA" && m_alg != "SM2")
        return CHECK_ERROR(CALL_E_INVALID_CALL);

    obj_ptr<PKey> to_key = (PKey*)key;
    if (key) {
        result_t hr;
        bool priv;

        mbedtls_pk_type_t type = mbedtls_pk_get_type(&to_key->m_key);
        if (type != MBEDTLS_PK_ECKEY && type != MBEDTLS_PK_SM2)
            return CHECK_ERROR(CALL_E_INVALIDARG);

        hr = to_key->isPrivate(priv);
        if (hr < 0)
            return hr;

        if (!priv)
            return CHECK_ERROR(CALL_E_INVALIDARG);

        mbedtls_ecp_keypair* ecp1 = mbedtls_pk_ec(m_key);
        mbedtls_ecp_keypair* ecp2 = mbedtls_pk_ec(to_key->m_key);
        if (ecp1->grp.id != ecp2->grp.id)
            return CHECK_ERROR(Runtime::setError("Public key is not valid for specified curve"));
    }

    int32_t ret;
    exlib::string str;
    exlib::string strsign;

    data->toString(str);
    sign->toString(strsign);

    ret = ecsdsa_verify(mbedtls_pk_ec(m_key), m_alg == "ECSDSA", key ? mbedtls_pk_ec(to_key->m_key) : NULL,
        (const unsigned char*)str.c_str(), str.length(), (const unsigned char*)strsign.c_str(), strsign.length(),
        mbedtls_ctr_drbg_random, &g_ssl.ctr_drbg);
    if (ret == MBEDTLS_ERR_ECP_VERIFY_FAILED || ret == MBEDTLS_ERR_RSA_VERIFY_FAILED || ret == MBEDTLS_ERR_SM2_BAD_SIGNATURE) {
        retVal = false;
        return 0;
    }

    if (ret != 0)
        return CHECK_ERROR(_ssl::setError(ret));

    retVal = true;

    return 0;
}

result_t PKey_ecc::computeSecret(PKey_base* publicKey, obj_ptr<Buffer_base>& retVal, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    result_t hr;
    bool priv;
    mbedtls_pk_type_t type;

    obj_ptr<PKey> pubkey = (PKey*)publicKey;
    type = mbedtls_pk_get_type(&pubkey->m_key);
    if (type != MBEDTLS_PK_ECKEY && type != MBEDTLS_PK_SM2)
        return CHECK_ERROR(CALL_E_INVALIDARG);

    type = mbedtls_pk_get_type(&m_key);
    if (type != MBEDTLS_PK_ECKEY && type != MBEDTLS_PK_SM2)
        return CHECK_ERROR(CALL_E_INVALID_CALL);

    hr = isPrivate(priv);
    if (hr < 0)
        return hr;

    if (!priv)
        return CHECK_ERROR(CALL_E_INVALID_CALL);

    mbedtls_ecp_keypair* ecp1 = mbedtls_pk_ec(m_key);
    mbedtls_ecp_keypair* ecp2 = mbedtls_pk_ec(pubkey->m_key);
    if (ecp1->grp.id != ecp2->grp.id)
        return CHECK_ERROR(Runtime::setError("Public key is not valid for specified curve"));

    int32_t ret;
    mbedtls_mpi z;
    mbedtls_mpi_init(&z);
    ret = mbedtls_ecdh_compute_shared(&ecp1->grp, &z, &ecp2->Q, &ecp1->d,
        mbedtls_ctr_drbg_random, &g_ssl.ctr_drbg);
    if (ret != 0)
        return CHECK_ERROR(_ssl::setError(ret));

    exlib::string data;
    int32_t sz = (int32_t)mbedtls_mpi_size(&z);

    data.resize(sz);
    mbedtls_mpi_write_binary(&z, (unsigned char*)data.c_buffer(), sz);
    mbedtls_mpi_free(&z);

    retVal = new Buffer(data);

    return 0;
}

result_t PKey_ecc::get_curve(exlib::string& retVal)
{
    mbedtls_ecp_keypair* ecp = mbedtls_pk_ec(m_key);
    const char* _name = get_curve_name(ecp->grp.id);
    if (_name)
        retVal = _name;

    return 0;
}

result_t PKey_ecc::get_keySize(int32_t& retVal)
{
    int32_t id = mbedtls_pk_ec(m_key)->grp.id;

    if (id < MBEDTLS_ECP_DP_ED25519)
        return PKey::get_keySize(retVal);

    retVal = 256;

    return 0;
}

result_t PKey_ecc::set_alg(exlib::string newVal)
{
    int32_t id = mbedtls_pk_ec(m_key)->grp.id;

    if (id >= MBEDTLS_ECP_DP_ED25519)
        return PKey::set_alg(newVal);
    else if (id == MBEDTLS_ECP_DP_SM2P256R1) {
        if (newVal != "SM2" && newVal != "ECSDSA")
            return CHECK_ERROR(CALL_E_INVALIDARG);
        m_alg = newVal;
    } else {
        if (newVal != "ECDSA" && newVal != "ECSDSA")
            return CHECK_ERROR(CALL_E_INVALIDARG);
        m_alg = newVal;
    }

    return 0;
}

}