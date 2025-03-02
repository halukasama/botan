/*
* SipHash
* (C) 2014,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/siphash.h>
#include <botan/internal/loadstor.h>
#include <botan/internal/rotate.h>
#include <botan/internal/fmt.h>

namespace Botan {

namespace {

void SipRounds(uint64_t M, secure_vector<uint64_t>& V, size_t r)
   {
   uint64_t V0 = V[0], V1 = V[1], V2 = V[2], V3 = V[3];

   V3 ^= M;
   for(size_t i = 0; i != r; ++i)
      {
      V0 += V1; V2 += V3;
      V1 = rotl<13>(V1);
      V3 = rotl<16>(V3);
      V1 ^= V0; V3 ^= V2;
      V0 = rotl<32>(V0);

      V2 += V1; V0 += V3;
      V1 = rotl<17>(V1);
      V3 = rotl<21>(V3);
      V1 ^= V2; V3 ^= V0;
      V2 = rotl<32>(V2);
      }
   V0 ^= M;

   V[0] = V0; V[1] = V1; V[2] = V2; V[3] = V3;
   }

}

void SipHash::add_data(const uint8_t input[], size_t length)
   {
   assert_key_material_set();

   // SipHash counts the message length mod 256
   m_words += static_cast<uint8_t>(length);

   if(m_mbuf_pos)
      {
      while(length && m_mbuf_pos != 8)
         {
         m_mbuf = (m_mbuf >> 8) | (static_cast<uint64_t>(input[0]) << 56);
         ++m_mbuf_pos;
         ++input;
         length--;
         }

      if(m_mbuf_pos == 8)
         {
         SipRounds(m_mbuf, m_V, m_C);
         m_mbuf_pos = 0;
         m_mbuf = 0;
         }
      }

   while(length >= 8)
      {
      SipRounds(load_le<uint64_t>(input, 0), m_V, m_C);
      input += 8;
      length -= 8;
      }

   for(size_t i = 0; i != length; ++i)
      {
      m_mbuf = (m_mbuf >> 8) | (static_cast<uint64_t>(input[i]) << 56);
      m_mbuf_pos++;
      }
   }

void SipHash::final_result(uint8_t mac[])
   {
   assert_key_material_set();

   if(m_mbuf_pos == 0)
      {
      m_mbuf = (static_cast<uint64_t>(m_words) << 56);
      }
   else if(m_mbuf_pos < 8)
      {
      m_mbuf = (m_mbuf >> (64-m_mbuf_pos*8)) | (static_cast<uint64_t>(m_words) << 56);
      }

   SipRounds(m_mbuf, m_V, m_C);

   m_V[2] ^= 0xFF;
   SipRounds(0, m_V, m_D);

   const uint64_t X = m_V[0] ^ m_V[1] ^ m_V[2] ^ m_V[3];

   store_le(X, mac);

   m_V[0] = m_K[0] ^ 0x736F6D6570736575;
   m_V[1] = m_K[1] ^ 0x646F72616E646F6D;
   m_V[2] = m_K[0] ^ 0x6C7967656E657261;
   m_V[3] = m_K[1] ^ 0x7465646279746573;
   m_mbuf = 0;
   m_mbuf_pos = 0;
   m_words = 0;
   }

bool SipHash::has_keying_material() const
   {
   return !m_V.empty();
   }

void SipHash::key_schedule(const uint8_t key[], size_t /*length*/)
   {
   const uint64_t K0 = load_le<uint64_t>(key, 0);
   const uint64_t K1 = load_le<uint64_t>(key, 1);

   m_K.resize(2);
   m_K[0] = K0;
   m_K[1] = K1;

   m_V.resize(4);
   m_V[0] = m_K[0] ^ 0x736F6D6570736575;
   m_V[1] = m_K[1] ^ 0x646F72616E646F6D;
   m_V[2] = m_K[0] ^ 0x6C7967656E657261;
   m_V[3] = m_K[1] ^ 0x7465646279746573;
   }

void SipHash::clear()
   {
   zap(m_K);
   zap(m_V);
   m_mbuf = 0;
   m_mbuf_pos = 0;
   m_words = 0;
   }

std::string SipHash::name() const
   {
   return fmt("SipHash({},{})", m_C, m_D);
   }

std::unique_ptr<MessageAuthenticationCode> SipHash::new_object() const
   {
   return std::make_unique<SipHash>(m_C, m_D);
   }

}
