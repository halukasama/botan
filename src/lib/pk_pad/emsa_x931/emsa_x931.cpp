/*
* EMSA_X931
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/emsa_x931.h>
#include <botan/exceptn.h>
#include <botan/internal/hash_id.h>

namespace Botan {

namespace {

std::vector<uint8_t> emsa2_encoding(const std::vector<uint8_t>& msg,
                                   size_t output_bits,
                                   const std::vector<uint8_t>& empty_hash,
                                   uint8_t hash_id)
   {
   const size_t HASH_SIZE = empty_hash.size();

   size_t output_length = (output_bits + 1) / 8;

   if(msg.size() != HASH_SIZE)
      throw Encoding_Error("EMSA_X931::encoding_of: Bad input length");
   if(output_length < HASH_SIZE + 4)
      throw Encoding_Error("EMSA_X931::encoding_of: Output length is too small");

   const bool empty_input = (msg == empty_hash);

   std::vector<uint8_t> output(output_length);

   output[0] = (empty_input ? 0x4B : 0x6B);
   output[output_length - 3 - HASH_SIZE] = 0xBA;
   set_mem(&output[1], output_length - 4 - HASH_SIZE, 0xBB);
   buffer_insert(output, output_length - (HASH_SIZE + 2), msg.data(), msg.size());
   output[output_length-2] = hash_id;
   output[output_length-1] = 0xCC;

   return output;
   }

}

std::string EMSA_X931::name() const
   {
   return "EMSA2(" + m_hash->name() + ")";
   }

void EMSA_X931::update(const uint8_t input[], size_t length)
   {
   m_hash->update(input, length);
   }

std::vector<uint8_t> EMSA_X931::raw_data()
   {
   return m_hash->final_stdvec();
   }

/*
* EMSA_X931 Encode Operation
*/
std::vector<uint8_t> EMSA_X931::encoding_of(const std::vector<uint8_t>& msg,
                                      size_t output_bits,
                                      RandomNumberGenerator& /*rng*/)
   {
   return emsa2_encoding(msg, output_bits, m_empty_hash, m_hash_id);
   }

/*
* EMSA_X931 Verify Operation
*/
bool EMSA_X931::verify(const std::vector<uint8_t>& coded,
                   const std::vector<uint8_t>& raw,
                   size_t key_bits)
   {
   try
      {
      return (coded == emsa2_encoding(raw, key_bits,
                                      m_empty_hash, m_hash_id));
      }
   catch(...)
      {
      return false;
      }
   }

/*
* EMSA_X931 Constructor
*/
EMSA_X931::EMSA_X931(std::unique_ptr<HashFunction> hash) :
   m_hash(std::move(hash))
   {
   m_empty_hash = m_hash->final_stdvec();

   m_hash_id = ieee1363_hash_id(m_hash->name());

   if(!m_hash_id)
      throw Encoding_Error("EMSA_X931 no hash identifier for " + m_hash->name());
   }

}
