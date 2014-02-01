#include <buffer.h>
#include <stage.h>

#include <unistd.h>

using namespace os;
using namespace media;

#include <iostream>
using namespace std;

Buffer::Buffer( Stage *pcStage, int nOutput )
{
	/* We hold a pointer to the associated stage but we do not own it */
	m_pcStage = pcStage;
	m_nOutput = nOutput;

	m_nMin = 5;
	m_nMax = 10;

	m_hLock = create_semaphore( "buffer_lock", 1, SEMSTYLE_COUNTING );
	m_hWait = create_semaphore( "buffer_wait", 0, SEMSTYLE_COUNTING );
	m_hCount = create_semaphore( "buffer_count", 0, SEMSTYLE_COUNTING );

	m_pcThread = new BufferThread( this );
	m_bIsRunning = false;
	m_bCanFill = true;
}

Buffer::~Buffer()
{
	if( m_bIsRunning )
		m_pcThread->Stop();
	/* XXXKV: Can we terminate a thread that has never been started? */
	m_pcThread->Terminate();

	delete_semaphore( m_hCount );
	delete_semaphore( m_hWait );
	delete_semaphore( m_hLock );
}

status_t Buffer::SetMinMax( unsigned int nMin, unsigned int nMax )
{
	if( ( nMin < 1 || nMax < 1 ) || ( nMax < nMin ) )
		return EINVAL;

	m_nMin = nMin;
	m_nMax = nMax;

	return EOK;
}

status_t Buffer::Start( void )
{
	lock_semaphore( m_hLock );

	if( false == m_bIsRunning )
	{
		m_pcThread->Start();
		m_bIsRunning = true;
	}
	unlock_semaphore( m_hLock );

	return EOK;
}

status_t Buffer::Stop( void )
{
	lock_semaphore( m_hLock );

	if( m_bIsRunning )
	{
		m_pcThread->Stop();
		m_bIsRunning = false;
	}
	unlock_semaphore( m_hLock );

	return EOK;
}

Packet * Buffer::GetPacket( bool bNoBlock, bool bGet )
{
	if( ( bNoBlock || ( m_bCanFill == false ) ) && GetCount() == 0  )
		return NULL;

	/* Wait for a packet */
	lock_semaphore( m_hCount );

	/* Take the oldest packet from the front of the queue */
	lock_semaphore( m_hLock );

	Packet *pcPacket = m_vpcQueue.front();
	if( bGet )
		m_vpcQueue.pop();
	else
		unlock_semaphore( m_hCount );

	/* If we're below the threshold, start re-filling the buffer */
	if( GetCount() < m_nMin )
		wakeup_sem( m_hWait, true );

	unlock_semaphore( m_hLock );

	return pcPacket;
}

size_t Buffer::GetCount( void )
{
	return get_semaphore_count( m_hCount );
}

Buffer::BufferThread::BufferThread( Buffer *pcParent ) : Thread( "buffer_thread", DISPLAY_PRIORITY, 1024 )
{
	m_pcParent = pcParent;
}

Buffer::BufferThread::~BufferThread()
{

}

int32 Buffer::BufferThread::Run( void )
{
	Stage *pcStage = m_pcParent->m_pcStage;
	int nOutput = m_pcParent->m_nOutput;
	sem_id hLock = m_pcParent->m_hLock;
	sem_id hWait = m_pcParent->m_hWait;

	while( true )
	{
		Packet *pcPacket;

		/* Add a new packet to the end of the queue */
		if( pcStage->GetPacket( &pcPacket, nOutput ) != EOK )
		{
			m_pcParent->m_bCanFill = false;
			Terminate();
		}

		lock_semaphore( hLock );
		m_pcParent->m_vpcQueue.push( pcPacket );

		/* Increment the count */
		unlock_semaphore( m_pcParent->m_hCount );

		if( m_pcParent->GetCount() == m_pcParent->m_nMax )
		{
			//cerr << "unlock_and_suspend" << endl;
			unlock_and_suspend( hWait, hLock );
		}
		else
			unlock_semaphore( hLock );
	}

	return 0;
}

