/*
* (C) 2015,2016 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include "fuzzers.h"
#include <botan/tls_client.h>
#include <botan/tls_session_manager_noop.h>

class Fuzzer_TLS_Client_Creds : public Botan::Credentials_Manager
   {
   public:
      std::string psk_identity_hint(const std::string&, const std::string&) override { return "psk_hint"; }
      std::string psk_identity(const std::string&, const std::string&, const std::string&) override { return "psk_id"; }
      Botan::SymmetricKey psk(const std::string&, const std::string&, const std::string&) override
         {
         return Botan::SymmetricKey("AABBCCDDEEFF00112233445566778899");
         }
   };

class Fuzzer_TLS_Policy : public Botan::TLS::Policy
   {
   public:
      std::vector<uint16_t> ciphersuite_list(Botan::TLS::Protocol_Version) const override
         {
         std::vector<uint16_t> ciphersuites;

         for(auto&& suite : Botan::TLS::Ciphersuite::all_known_ciphersuites())
            {
            if(suite.valid() == false)
               ciphersuites.push_back(suite.ciphersuite_code());
            }

         return ciphersuites;
         }
   };

class Fuzzer_TLS_Client_Callbacks : public Botan::TLS::Callbacks
   {
   public:
       void tls_emit_data(std::span<const uint8_t>) override
         {
         // discard
         }

      void tls_record_received(uint64_t, std::span<const uint8_t>) override
         {
         // ignore peer data
         }

      void tls_alert(Botan::TLS::Alert) override
         {
         // ignore alert
         }

      void tls_verify_cert_chain(
         const std::vector<Botan::X509_Certificate>& cert_chain,
         const std::vector<std::optional<Botan::OCSP::Response>>& ocsp_responses,
         const std::vector<Botan::Certificate_Store*>& trusted_roots,
         Botan::Usage_Type usage,
         const std::string& hostname,
         const Botan::TLS::Policy& policy) override
         {
         try
            {
            // try to validate to exercise those code paths
            Botan::TLS::Callbacks::tls_verify_cert_chain(cert_chain, ocsp_responses,
                                                         trusted_roots, usage, hostname, policy);
            }
         catch(...)
            {
            // ignore validation result
            }
         }

   };

void fuzz(const uint8_t in[], size_t len)
   {
   if(len == 0)
      return;

   auto session_manager = std::make_shared<Botan::TLS::Session_Manager_Noop>();
   auto policy = std::make_shared<Fuzzer_TLS_Policy>();
   Botan::TLS::Protocol_Version client_offer = Botan::TLS::Protocol_Version::TLS_V12;
   Botan::TLS::Server_Information info("server.name", 443);
   auto callbacks = std::make_shared<Fuzzer_TLS_Client_Callbacks>();
   auto creds = std::make_shared<Fuzzer_TLS_Client_Creds>();

   Botan::TLS::Client client(callbacks,
                             session_manager,
                             creds,
                             policy,
                             fuzzer_rng_as_shared(),
                             info,
                             client_offer);

   try
      {
      client.received_data(in, len);
      }
   catch(std::exception& e)
      {
      }

   }

