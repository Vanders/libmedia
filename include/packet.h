#ifndef __F_MEDIA_PACKET_H_
#define __F_MEDIA_PACKET_H_

#include <atheos/kdebug.h>
#include <string.h>

namespace media
{

class PacketInfo
{
	public:
		PacketInfo(){};
};

typedef enum audio_format
{
	UNKNOWN,
	PCM_UNSIGNED_8,
/*	PCM_SIGNED_8, */
	PCM_UNSIGNED_LE,
	PCM_UNSIGNED_BE,
	PCM_SIGNED_LE,
	PCM_SIGNED_BE,
	OTHER
} audio_format_t;

class AudioPacketInfo : public PacketInfo
{
	public:
		AudioPacketInfo(){};

		audio_format_t eFormat;
		uint32 nChannels;
		uint32 nSampleRate;
		uint32 nBitsPerSample;
};

class Packet
{
	public:
		typedef enum PacketType_e
		{
			UNKNOWN,
			AUDIO,
			VIDEO,
			SUBTITLE,
			OTHER
		} PacketType;

		Packet( void )
		{
			m_eType = UNKNOWN;
			m_pcInfo = NULL;
			m_pData = NULL;
			m_nSize = 0;
		};
		Packet( const uint8 *pData, const size_t nSize, PacketType eType = UNKNOWN, PacketInfo *pcInfo = NULL )
		{
			m_eType = eType;
			m_pcInfo = pcInfo;
			m_pData = NULL;
			m_nSize = 0;

			SetData( pData, nSize );
		};
		~Packet( void )
		{
			if( m_pcInfo )
				delete( m_pcInfo );

			if( m_pData )
				delete m_pData;
		};

		PacketType GetType( void ){ return m_eType; };
		void SetType( PacketType eType ){ m_eType = eType; };

		PacketInfo * GetInfo( void ){ return m_pcInfo; };
		void SetInfo( PacketInfo *pcInfo ){ m_pcInfo = pcInfo; };

		size_t GetDataSize( void ){ return m_nSize; };
		const uint8 * GetData( void ){ return m_pData; };
		void SetData( const uint8 *pData, const size_t nSize )
		{
			if( m_pData )
				delete m_pData;
			m_nSize = nSize;
			if( m_nSize > 0 )
			{
				m_pData = new uint8[m_nSize];
				m_pData = (uint8*)memcpy( m_pData, pData, m_nSize );
			}
			else
				m_pData = NULL;
		};

		Packet & operator=( const Packet &cPacket )
		{
			m_eType = cPacket.m_eType;
			SetData( cPacket.m_pData, cPacket.m_nSize );
			return( *this );
		};
	private:
		PacketType m_eType;
		PacketInfo *m_pcInfo;
		uint8 *m_pData;
		size_t m_nSize;
};

}

#endif	/* __F_MEDIA_PACKET_H_ */

