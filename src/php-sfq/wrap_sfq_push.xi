  <function name="wrap_sfq_push" role="public">
    <proto>int wrap_sfq_push(string querootdir, string quename, array ioparam)</proto>
    <summary>libsfq:sfq_push() wrapper</summary>
    <code><?data
// start
	zval** entry = NULL;

	int push_rc = SFQ_RC_UNKNOWN;
	struct sfq_value val;

/* init */
	bzero(&val, sizeof(val));

	zend_hash_internal_pointer_reset(ioparam_hash);

	while (zend_hash_get_current_data(ioparam_hash, (void**)&entry) == SUCCESS)
	{
		char* str_key = NULL;
		ulong num_key = 0;
		int hkey_type = 0;
		int hval_type = 0;

	// 要素のキーを取得
		hkey_type = zend_hash_get_current_key(ioparam_hash, &str_key, &num_key, 0);

		if (hkey_type != HASH_KEY_IS_STRING)
		{
			goto CONTINUE_LABEL;
		}

		if (! str_key)
		{
			goto CONTINUE_LABEL;
		}

	// 文字列要素の値を取得

		hval_type = Z_TYPE_PP(entry);

		switch (hval_type)
		{
			case IS_STRING:
			{
				char* str_val = Z_STRVAL_PP(entry);

				if (strcasecmp("execpath", str_key) == 0)
				{
					val.execpath = str_val;
				}
				else if (strcasecmp("execargs", str_key) == 0)
				{
					val.execargs = str_val;
				}
				else if (strcasecmp("metatext", str_key) == 0)
				{
					val.metatext = str_val;
				}
				else if (strcasecmp("soutpath", str_key) == 0)
				{
					val.soutpath = str_val;
				}
				else if (strcasecmp("serrpath", str_key) == 0)
				{
					val.serrpath = str_val;
				}
				else if (strcasecmp("payload", str_key) == 0)
				{
					if (! val.payload_size)
					{
						val.payload_size = Z_STRLEN_PP(entry);
					}
					val.payload = (sfq_byte*)str_val;
				}

				break;
			}

			case IS_LONG:
			{
				long long_val = Z_LVAL_PP(entry);

				if (strcasecmp("payload_size", str_key) == 0)
				{
					val.payload_size = (size_t)long_val;
				}
				else if (strcasecmp("payload_type", str_key) == 0)
				{
					val.payload_type = (payload_type_t)long_val;
				}

				break;
			}
		}


CONTINUE_LABEL:
		zend_hash_move_forward(ioparam_hash);
	}

	push_rc = sfq_push(querootdir, quename, &val);

	if (push_rc == SFQ_RC_SUCCESS)
	{
		char uuid_s[36 + 1] = "";

		uuid_unparse(val.uuid, uuid_s);
/*
		const char hkey_uuid[] = "uuid";
		zval* z_uuid = NULL;

		MAKE_STD_ZVAL(z_uuid);
		ZVAL_STRING(z_uuid, uuid_s, 1);

		zend_hash_update(ioparam_hash, hkey_uuid, sizeof(hkey_uuid), &z_uuid, sizeof(*z_uuid), NULL);
*/
	// uuid
		SFQWL_ZH_UPDATE_STRING(ioparam_hash, "uuid", uuid_s);

	// payload_size
/*
payload_size は push 前に算出している可能性があるので書き戻す
*/
		SFQWL_ZH_UPDATE_LONG(ioparam_hash, "payload_size", val.payload_size);
	}

/*
val に設定されているものは、全て php のメモリ領域なので開放する必要がない

	sfq_free_value(&val);
*/

	RETURN_LONG(push_rc);
// end
    ?></code>

    <test>
      <code><?data
$ioparam = [ 'execpath' => 'cc', 'execargs' => 'dd' ];
echo wrap_sfq_push(null, null, $ioparam);
      ?></code>
      <result mode="plain"><?data
0
      ?></result>
    </test>
  </function>

