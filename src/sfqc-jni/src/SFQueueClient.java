package jp.co.iret.sfq;

import java.util.Map;

public abstract class SFQueueClient implements SFQueueClientInterface
{
	static final int SFQ_RC_SUCCESS		= 0;
	static final int SFQ_RC_FATAL_MIN	= 21;

	int lastError_ = 0;
	String lastMessage_ = "";

	public void setLastError(int arg)
	{
		lastError_ = arg;
	}

	public int lastError()
	{
		return lastError_;
	}

	public void setLastMessage(final String arg)
	{
		lastMessage_ = arg;
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

	@Override
	public Map<String, Object> pop() throws SFQueueClientException
	{
		return pop(null);
	}
	@Override
	public Map<String, Object> shift() throws SFQueueClientException
	{
		return shift(null);
	}
}
