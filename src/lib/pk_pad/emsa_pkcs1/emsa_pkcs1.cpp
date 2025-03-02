/*
* PKCS #1 v1.5 signature padding
* (C) 1999-2008 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/emsa_pkcs1.h>
#include <botan/internal/hash_id.h>
#include <botan/exceptn.h>

namespace Botan {

namespace {

std::vector<uint8_t> emsa3_encoding(const std::vector<uint8_t>& msg,
                                   size_t output_bits,
                                   const uint8_t hash_id[],
                                   size_t hash_id_length)
   {
   size_t output_length = output_bits / 8;
   if(output_length < hash_id_length + msg.size() + 10)
      throw Encoding_Error("emsa3_encoding: Output length is too small");

   std::vector<uint8_t> T(output_length);
   const size_t P_LENGTH = output_length - msg.size() - hash_id_length - 2;

   T[0] = 0x01;
   set_mem(&T[1], P_LENGTH, 0xFF);
   T[P_LENGTH+1] = 0x00;

   if(hash_id_length > 0)
      {
      BOTAN_ASSERT_NONNULL(hash_id);
      buffer_insert(T, P_LENGTH+2, hash_id, hash_id_length);
      }

   buffer_insert(T, output_length-msg.size(), msg.data(), msg.size());
   return T;
   }

}

void EMSA_PKCS1v15::update(const uint8_t input[], size_t length)
   {
   m_hash->update(input, length);
   }

std::vector<uint8_t> EMSA_PKCS1v15::raw_data()
   {
   return m_hash->final_stdvec();
   }

std::vector<uint8_t>
EMSA_PKCS1v15::encoding_of(const std::vector<uint8_t>& msg,
                           size_t output_bits,
                           RandomNumberGenerator& /*rng*/)
   {
   if(msg.size() != m_hash->output_length())
      throw Encoding_Error("EMSA_PKCS1v15::encoding_of: Bad input length");

   return emsa3_encoding(msg, output_bits,
                         m_hash_id.data(), m_hash_id.size());
   }

bool EMSA_PKCS1v15::verify(const std::vector<uint8_t>& coded,
                           const std::vector<uint8_t>& raw,
                           size_t key_bits)
   {
   if(raw.size() != m_hash->output_length())
      return false;

   try
      {
      return (coded == emsa3_encoding(raw, key_bits,
                                      m_hash_id.data(), m_hash_id.size()));
      }
   catch(...)
      {
      return false;
      }
   }

EMSA_PKCS1v15::EMSA_PKCS1v15(std::unique_ptr<HashFunction> hash) :
   m_hash(std::move(hash))
   {
   m_hash_id = pkcs_hash_id(m_hash->name());
   }

EMSA_PKCS1v15_Raw::EMSA_PKCS1v15_Raw()
   {
   m_hash_output_len = 0;
   // m_hash_id, m_hash_name left empty
   }

EMSA_PKCS1v15_Raw::EMSA_PKCS1v15_Raw(const std::string& hash_algo)
   {
   std::unique_ptr<HashFunction> hash(HashFunction::create_or_throw(hash_algo));
   m_hash_id = pkcs_hash_id(hash_algo);
   m_hash_name = hash->name();
   m_hash_output_len = hash->output_length();
   }

void EMSA_PKCS1v15_Raw::update(const uint8_t input[], size_t length)
   {
   m_message += std::make_pair(input, length);
   }

std::vector<uint8_t> EMSA_PKCS1v15_Raw::raw_data()
   {
   std::vector<uint8_t> ret;
   std::swap(ret, m_message);

   if(m_hash_output_len > 0 && ret.size() != m_hash_output_len)
      throw Encoding_Error("EMSA_PKCS1v15_Raw::encoding_of: Bad input length");

   return ret;
   }

std::vector<uint8_t>
EMSA_PKCS1v15_Raw::encoding_of(const std::vector<uint8_t>& msg,
                               size_t output_bits,
                               RandomNumberGenerator& /*rng*/)
   {
   return emsa3_encoding(msg, output_bits, m_hash_id.data(), m_hash_id.size());
   }

bool EMSA_PKCS1v15_Raw::verify(const std::vector<uint8_t>& coded,
                               const std::vector<uint8_t>& raw,
                               size_t key_bits)
   {
   if(m_hash_output_len > 0 && raw.size() != m_hash_output_len)
      return false;

   try
      {
      return (coded == emsa3_encoding(raw, key_bits, m_hash_id.data(), m_hash_id.size()));
      }
   catch(...)
      {
      return false;
      }
   }

}
