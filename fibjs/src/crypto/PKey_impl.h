/*
 * PKey_impl.h
 *
 *  Created on: Apr 30, 2022
 *      Author: lion
 */

#include "PKey.h"

namespace fibjs {

class PKey_rsa : public PKey {
public:
    PKey_rsa(int32_t size);
    PKey_rsa(mbedtls_pk_context& key);

public:
    // PKey
    virtual result_t get_publicKey(obj_ptr<PKey_base>& retVal);
    virtual result_t isPrivate(bool& retVal);
    virtual result_t clone(obj_ptr<PKey_base>& retVal);
    virtual result_t equal(PKey_base* key, bool& retVal);

public:
    static result_t generateKey(int32_t size, obj_ptr<PKey_base>& retVal);
};

class PKey_ecc : public PKey {
protected:
    PKey_ecc();

public:
    PKey_ecc(int32_t id);
    PKey_ecc(mbedtls_pk_context& key);

public:
    // PKey
    virtual result_t get_curve(exlib::string& retVal);
    virtual result_t get_keySize(int32_t& retVal);
    virtual result_t set_alg(exlib::string newVal);
    virtual result_t get_publicKey(obj_ptr<PKey_base>& retVal);
    virtual result_t isPrivate(bool& retVal);
    virtual result_t clone(obj_ptr<PKey_base>& retVal);
    virtual result_t equal(PKey_base* key, bool& retVal);
    virtual result_t sign(Buffer_base* data, int32_t alg, obj_ptr<Buffer_base>& retVal, AsyncEvent* ac);
    virtual result_t sign(Buffer_base* data, PKey_base* key, obj_ptr<Buffer_base>& retVal, AsyncEvent* ac);
    virtual result_t verify(Buffer_base* data, Buffer_base* sign, int32_t alg, bool& retVal, AsyncEvent* ac);
    virtual result_t verify(Buffer_base* data, Buffer_base* sign, PKey_base* key, bool& retVal, AsyncEvent* ac);
    virtual result_t computeSecret(PKey_base* publicKey, obj_ptr<Buffer_base>& retVal, AsyncEvent* ac);

public:
    static PKey_ecc* create(mbedtls_pk_context& key);
    static result_t generateKey(exlib::string curve, obj_ptr<PKey_base>& retVal);

public:
    static int32_t get_curve_id(exlib::string& curve);
    static const char* get_curve_name(int32_t id);
};

class PKey_25519 : public PKey_ecc {
public:
    PKey_25519();
    PKey_25519(mbedtls_pk_context& key);

public:
    // PKey
    virtual result_t pem(exlib::string& retVal);
    virtual result_t der(obj_ptr<Buffer_base>& retVal);
    virtual result_t sign(Buffer_base* data, int32_t alg, obj_ptr<Buffer_base>& retVal, AsyncEvent* ac);
    virtual result_t verify(Buffer_base* data, Buffer_base* sign, int32_t alg, bool& retVal, AsyncEvent* ac);

public:
    static result_t from(Buffer_base* DerKey, obj_ptr<PKey_base>& retVal);
    static result_t from(exlib::string pemKey, obj_ptr<PKey_base>& retVal);
};

class PKey_bls_g1 : public PKey_ecc {
public:
    PKey_bls_g1();
    PKey_bls_g1(mbedtls_pk_context& key);

public:
    // PKey
    virtual result_t sign(Buffer_base* data, int32_t alg, obj_ptr<Buffer_base>& retVal, AsyncEvent* ac);
    virtual result_t verify(Buffer_base* data, Buffer_base* sign, int32_t alg, bool& retVal, AsyncEvent* ac);
};

class PKey_bls_g2 : public PKey_ecc {
public:
    PKey_bls_g2();
    PKey_bls_g2(mbedtls_pk_context& key);

public:
    // PKey
    virtual result_t sign(Buffer_base* data, int32_t alg, obj_ptr<Buffer_base>& retVal, AsyncEvent* ac);
    virtual result_t verify(Buffer_base* data, Buffer_base* sign, int32_t alg, bool& retVal, AsyncEvent* ac);
};

}