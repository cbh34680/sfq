package jp.co.iret.sfq;

public interface SFQueueClientInterface
{
	public String push(java.util.Map<String, Object> params) throws SFQueueClientException;
	public java.util.Map<String, Object> pop() throws SFQueueClientException;
	public java.util.Map<String, Object> shift() throws SFQueueClientException;
}
