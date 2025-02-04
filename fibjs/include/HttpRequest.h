/*
 * HttpRequest.h
 *
 *  Created on: Aug 9, 2012
 *      Author: lion
 */

#pragma once

#include "ifs/HttpRequest.h"
#include "HttpMessage.h"
#include "HttpResponse.h"
#include "HttpCollection.h"

namespace fibjs {

class HttpRequest : public HttpRequest_base {
public:
    HttpRequest()
    {
        m_message = new HttpMessage();
        clear();
        extMemory(4096);
    }

public:
    // Message_base
    virtual result_t get_value(exlib::string& retVal);
    virtual result_t set_value(exlib::string newVal);
    virtual result_t get_params(obj_ptr<NArray>& retVal);
    virtual result_t get_type(int32_t& retVal);
    virtual result_t set_type(int32_t newVal);
    virtual result_t get_data(v8::Local<v8::Value>& retVal);
    virtual result_t get_body(obj_ptr<SeekableStream_base>& retVal);
    virtual result_t set_body(SeekableStream_base* newVal);
    virtual result_t read(int32_t bytes, obj_ptr<Buffer_base>& retVal, AsyncEvent* ac);
    virtual result_t readAll(obj_ptr<Buffer_base>& retVal, AsyncEvent* ac);
    virtual result_t write(Buffer_base* data, AsyncEvent* ac);
    virtual result_t json(v8::Local<v8::Value> data, v8::Local<v8::Value>& retVal);
    virtual result_t json(v8::Local<v8::Value>& retVal);
    virtual result_t pack(v8::Local<v8::Value> data, v8::Local<v8::Value>& retVal);
    virtual result_t pack(v8::Local<v8::Value>& retVal);
    virtual result_t get_length(int64_t& retVal);
    virtual result_t end();
    virtual result_t isEnded(bool& retVal);
    virtual result_t clear();
    virtual result_t sendTo(Stream_base* stm, AsyncEvent* ac);
    virtual result_t readFrom(Stream_base* stm, AsyncEvent* ac);
    virtual result_t get_stream(obj_ptr<Stream_base>& retVal);
    virtual result_t get_lastError(exlib::string& retVal);
    virtual result_t set_lastError(exlib::string newVal);

public:
    // HttpMessage_base
    virtual result_t get_protocol(exlib::string& retVal);
    virtual result_t set_protocol(exlib::string newVal);
    virtual result_t get_headers(obj_ptr<HttpCollection_base>& retVal);
    virtual result_t get_keepAlive(bool& retVal);
    virtual result_t set_keepAlive(bool newVal);
    virtual result_t get_upgrade(bool& retVal);
    virtual result_t set_upgrade(bool newVal);
    virtual result_t get_maxHeadersCount(int32_t& retVal);
    virtual result_t set_maxHeadersCount(int32_t newVal);
    virtual result_t get_maxHeaderSize(int32_t& retVal);
    virtual result_t set_maxHeaderSize(int32_t newVal);
    virtual result_t get_maxBodySize(int32_t& retVal);
    virtual result_t set_maxBodySize(int32_t newVal);
    virtual result_t get_socket(obj_ptr<Stream_base>& retVal);
    virtual result_t hasHeader(exlib::string name, bool& retVal);
    virtual result_t firstHeader(exlib::string name, exlib::string& retVal);
    virtual result_t allHeader(exlib::string name, obj_ptr<NObject>& retVal);
    virtual result_t addHeader(v8::Local<v8::Object> map);
    virtual result_t addHeader(exlib::string name, v8::Local<v8::Array> values);
    virtual result_t addHeader(exlib::string name, exlib::string value);
    virtual result_t setHeader(v8::Local<v8::Object> map);
    virtual result_t setHeader(exlib::string name, v8::Local<v8::Array> values);
    virtual result_t setHeader(exlib::string name, exlib::string value);
    virtual result_t removeHeader(exlib::string name);

public:
    // HttpRequest_base
    virtual result_t get_response(obj_ptr<HttpResponse_base>& retVal);
    virtual result_t get_method(exlib::string& retVal);
    virtual result_t set_method(exlib::string newVal);
    virtual result_t get_address(exlib::string& retVal);
    virtual result_t set_address(exlib::string newVal);
    virtual result_t get_queryString(exlib::string& retVal);
    virtual result_t set_queryString(exlib::string newVal);
    virtual result_t get_cookies(obj_ptr<HttpCollection_base>& retVal);
    virtual result_t get_form(obj_ptr<HttpCollection_base>& retVal);
    virtual result_t get_query(obj_ptr<HttpCollection_base>& retVal);

public:
    result_t addHeader(NObject* map)
    {
        for (int32_t i = 0; i < (int32_t)map->m_values.size(); i++) {
            NObject::Value& v = map->m_values[i];

            if (!v.m_val.isUndefined()) {
                obj_ptr<NArray> list = NArray::getInstance(v.m_val.object());

                if (list) {
                    for (int32_t i = 0; i < (int32_t)list->m_array.size(); i++)
                        addHeader(v.m_pos->first, list->m_array[i].string());
                } else
                    addHeader(v.m_pos->first, v.m_val.string());
            }
        }

        return 0;
    }

private:
    obj_ptr<HttpResponse_base> m_response;
    obj_ptr<HttpMessage> m_message;
    exlib::string m_method;
    exlib::string m_address;
    exlib::string m_queryString;
    obj_ptr<HttpCollection_base> m_cookies;
    obj_ptr<HttpCollection_base> m_query;
    obj_ptr<HttpCollection_base> m_form;
};

} /* namespace fibjs */
