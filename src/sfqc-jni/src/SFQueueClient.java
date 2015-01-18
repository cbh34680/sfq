package jp.co.iret.sfq;

public abstract class SFQueueClient implements SFQueueClientInterface
{
	static final int SFQ_RC_SUCCESS		= 0;
	static final int SFQ_RC_FATAL_MIN	= 21;

	SFQueueClient(boolean throwException)
	{
		throwException_ = throwException;
		notInitialized_ = false;
	}

	boolean throwException_;
	boolean notInitialized_;

	int lastError_ = 0;
	String lastMessage_ = "";

	protected void throwException(int code)
		throws SFQueueClientException
	{
		throwException(code, null);
	}

	protected void throwException(int code, String mesg)
		throws SFQueueClientException
	{
		lastError_ = code;

		lastMessage_ = (mesg == null)
			? SFQueueClientException.c2m(code)
			: mesg;

		if (throwException_)
		{
			throw new SFQueueClientException(code, mesg);
		}
	}

	public int lastError()
	{
		return lastError_;
	}

	public String lastMessage()
	{
		return lastMessage_;
	}

	public void clearLastError()
	{
		lastError_ = 0;
		lastMessage_ = "";
	}
}
