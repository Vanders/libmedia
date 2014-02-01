#ifndef __F_MEDIA_BUFFER_H_
#define __F_MEDIA_BUFFER_H_

#include <atheos/semaphore.h>
#include <util/thread.h>
#include <util/locker.h>
#include <queue>

namespace media
{

class Packet;
class Stage;

class Buffer
{
	public:
		Buffer( Stage *pcStage, int nOutput );
		~Buffer();

		status_t SetMinMax( unsigned int nMin, unsigned int nMax );

		status_t Start( void );
		status_t Stop( void );

		Packet * GetPacket( bool bNoBlock = false, bool bGet = true );
		size_t GetCount( void );

	private:
		class BufferThread : public os::Thread
		{
			public:
				BufferThread( Buffer *pcParent );
				~BufferThread();

				int32 Run( void );

			private:
				friend class Buffer;
				Buffer *m_pcParent;
		};
		friend class BufferThread;

		BufferThread *m_pcThread;
		sem_id m_hLock;
		sem_id m_hWait;

		bool m_bIsRunning;
		std::queue <Packet*> m_vpcQueue;

		Stage *m_pcStage;
		int m_nOutput;

		unsigned int m_nMin, m_nMax;
		sem_id m_hCount;

		bool m_bCanFill;
};

}

#endif	/* __F_MEDIA_BUFFER_H_ */
