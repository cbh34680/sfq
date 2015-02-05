package jp.co.iret.sfq;

import java.util.Map;

public interface SFQueueClientInterface
{
	public String push(Map<String, Object> params) throws SFQueueClientException;
	public Map<String, Object> pop(Map<String, Object> params) throws SFQueueClientException;
	public Map<String, Object> shift(Map<String, Object> params) throws SFQueueClientException;
	public Map<String, Object> pop() throws SFQueueClientException;
	public Map<String, Object> shift() throws SFQueueClientException;
}
