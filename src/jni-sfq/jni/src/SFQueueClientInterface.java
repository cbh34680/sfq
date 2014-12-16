package jp.co.iret.sfq;

public interface SFQueueClientInterface
{
	public String push_text(java.util.Map<String, Object> params);
	public String push_binary(java.util.Map<String, Object> params);

	public java.util.Map<String, Object> pop();
	public java.util.Map<String, Object> shift();
}

