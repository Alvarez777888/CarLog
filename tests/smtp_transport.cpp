// Transport regression check. curl options are captured; no mail is sent.
#include <curl/curl.h>
#include <cstdarg>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
static std::string captured_url, captured_protocols;
static CURLcode delivery_result = CURLE_OK;
static CURLcode capture_option(CURL *, CURLoption option, ...) {
    va_list args;
    va_start(args,option);
    if (option==CURLOPT_URL) captured_url=va_arg(args,const char *);
    if (option==CURLOPT_PROTOCOLS_STR) captured_protocols=va_arg(args,const char *);
    va_end(args);
    return CURLE_OK;
}
static CURLcode fake_delivery(CURL *) { return delivery_result; }
#undef curl_easy_setopt
#define curl_easy_setopt capture_option
#define curl_easy_perform fake_delivery
#include "../cpp/recovery.cpp"
int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    setenv("CARLOG_SMTP_USER","sender@example.com",1);
    setenv("CARLOG_SMTP_PASSWORD","test-only-password",1);
    setenv("CARLOG_SMTP_HOST","smtp.gmail.com",1);
    setenv("CARLOG_SMTP_PORT","587",1);
    assert(send_email("recipient@example.com","123456"));
    assert(captured_url=="smtp://smtp.gmail.com:587");
    assert(captured_protocols=="smtp,smtps");
    setenv("CARLOG_SMTP_PORT","465",1);
    assert(send_email("recipient@example.com","123456"));
    assert(captured_url=="smtps://smtp.gmail.com:465");
    delivery_result=CURLE_LOGIN_DENIED;
    assert(!send_email("recipient@example.com","123456"));
    curl_global_cleanup();
    std::cout<<"OK: STARTTLS 587, SMTPS 465 and delivery failure handling\n";
}
