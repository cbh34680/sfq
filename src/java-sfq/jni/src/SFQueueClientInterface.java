package jp.co.iret.sfq;

public interface SFQueueClientInterface
{
	public String push_text(java.util.HashMap<String, Object> params);
	public String push_binary(java.util.HashMap<String, Object> params);

	public java.util.HashMap<String, Object> pop();
	public java.util.HashMap<String, Object> shift();
}

