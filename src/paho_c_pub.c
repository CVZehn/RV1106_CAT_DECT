/*******************************************************************************
 * Copyright (c) 2012, 2020 IBM Corp., and others
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *   https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *    Guilherme Maciel Ferreira - add keep alive option
 *    Ian Craggs - add full capability
 *******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
 
 
#include "MQTTAsync.h"
#include "pubsub_c_opts.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/time.h>
#include <unistd.h>

#if defined(_WRS_KERNEL)
#include <OsWrapper.h>
#endif

volatile int toStop = 0;

struct pubsub_opts opts =
{
	1, 0, 0, 0, "\n", 100,  	/* debug/app options */
	NULL, NULL, 1, 0, 0, /* message options */
	MQTTVERSION_DEFAULT, NULL, "paho-c-pub", 0, 0, NULL, NULL, "localhost", "1883", NULL, 10, /* MQTT options */
	NULL, NULL, 0, 0, /* will options */
	0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* TLS options */
	0, {NULL, NULL}, /* MQTT V5 options */
	NULL, NULL, /* HTTP and HTTPS proxies */
};

MQTTAsync_responseOptions pub_opts = MQTTAsync_responseOptions_initializer;
MQTTProperty property;
MQTTProperties props = MQTTProperties_initializer;


void mysleep(int ms)
{
	#if defined(_WIN32)
		Sleep(ms);
	#else
		usleep(ms * 1000);
	#endif
}

void cfinish(int sig)
{
	signal(SIGINT, NULL);
	toStop = 1;
}


int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* m)
{
	/* not expecting any messages */
	return 1;
}


static int disconnected = 0;

void onDisconnect5(void* context, MQTTAsync_successData5* response)
{
	disconnected = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	disconnected = 1;
}


static int connected = 0;
void myconnect(MQTTAsync client);
int mypublish(MQTTAsync client, int datalen, char* data);

void onConnectFailure5(void* context, MQTTAsync_failureData5* response)
{
	fprintf(stderr, "Connect failed, rc %s reason code %s\n",
		MQTTAsync_strerror(response->code),
		MQTTReasonCode_toString(response->reasonCode));
	connected = -1;

	MQTTAsync client = (MQTTAsync)context;
}

void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	fprintf(stderr, "Connect failed, rc %s\n", response ? MQTTAsync_strerror(response->code) : "none");
	connected = -1;

	MQTTAsync client = (MQTTAsync)context;
}


void onConnect5(void* context, MQTTAsync_successData5* response)
{
}

void onConnect(void* context, MQTTAsync_successData* response)
{
}


static int published = 0;

void onPublishFailure5(void* context, MQTTAsync_failureData5* response)
{
	if (opts.verbose)
		fprintf(stderr, "Publish failed, rc %s reason code %s\n",
				MQTTAsync_strerror(response->code),
				MQTTReasonCode_toString(response->reasonCode));
	published = -1;
}

void onPublishFailure(void* context, MQTTAsync_failureData* response)
{
	if (opts.verbose)
		fprintf(stderr, "Publish failed, rc %s\n", MQTTAsync_strerror(response->code));
	published = -1;
}


void onPublish5(void* context, MQTTAsync_successData5* response)
{
	if (opts.verbose)
		printf("Publish succeeded, reason code %s\n",
				MQTTReasonCode_toString(response->reasonCode));

	if (opts.null_message || opts.message || opts.filename)
		toStop = 1;

	published = 1;
}


void onPublish(void* context, MQTTAsync_successData* response)
{
	if (opts.verbose)
		printf("Publish succeeded\n");

	if (opts.null_message || opts.message || opts.filename)
		toStop = 1;

	published = 1;
}


static int onSSLError(const char *str, size_t len, void *context)
{
	MQTTAsync client = (MQTTAsync)context;
	return fprintf(stderr, "SSL error: %s\n", str);
}

static unsigned int onPSKAuth(const char* hint,
                              char* identity,
                              unsigned int max_identity_len,
                              unsigned char* psk,
                              unsigned int max_psk_len,
                              void* context)
{
	int psk_len;
	int k, n;

	int rc = 0;
	struct pubsub_opts* opts = context;

	/* printf("Trying TLS-PSK auth with hint: %s\n", hint);*/

	if (opts->psk == NULL || opts->psk_identity == NULL)
	{
		/* printf("No PSK entered\n"); */
		goto exit;
	}

	/* psk should be array of bytes. This is a quick and dirty way to
	 * convert hex to bytes without input validation */
	psk_len = (int)strlen(opts->psk) / 2;
	if (psk_len > max_psk_len)
	{
		fprintf(stderr, "PSK too long\n");
		goto exit;
	}
	for (k=0, n=0; k < psk_len; k++, n += 2)
	{
		sscanf(&opts->psk[n], "%2hhx", &psk[k]);
	}

	/* identity should be NULL terminated string */
	strncpy(identity, opts->psk_identity, max_identity_len);
	if (identity[max_identity_len - 1] != '\0')
	{
		fprintf(stderr, "Identity too long\n");
		goto exit;
	}

	/* Function should return length of psk on success. */
	rc = psk_len;

exit:
	return rc;
}

void myconnect(MQTTAsync client)
{
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
	MQTTAsync_willOptions will_opts = MQTTAsync_willOptions_initializer;
	int rc = 0;

	printf("Connecting\n");
	MQTTAsync_connectOptions conn_opts5 = MQTTAsync_connectOptions_initializer5;
	conn_opts = conn_opts5;
	conn_opts.onSuccess5 = onConnect5;
	conn_opts.onFailure5 = onConnectFailure5;
	conn_opts.cleanstart = 1;
	conn_opts.keepAliveInterval = 100;
	conn_opts.username = "admin";
	conn_opts.password = "HMZB1527733";
	conn_opts.MQTTVersion = MQTTVERSION_5;
	conn_opts.context = client;
	conn_opts.automaticReconnect = 1;
	//conn_opts.httpProxy = opts.http_proxy;
	//conn_opts.httpsProxy = opts.https_proxy;
	opts.topic = "/livingroom/guard/cat/set/sw";
	will_opts.message = opts.will_payload;
	will_opts.topicName = opts.topic ;
	will_opts.qos = opts.will_qos;
	will_opts.retained = opts.will_retain;
	conn_opts.will = &will_opts;
	
	#if 0
	if (opts.will_topic) 	/* will options */
	{
		will_opts.message = opts.will_payload;
		will_opts.topicName = opts.will_topic;
		will_opts.qos = opts.will_qos;
		will_opts.retained = opts.will_retain;
		conn_opts.will = &will_opts;
	}

	if (opts.connection && (strncmp(opts.connection, "ssl://", 6) == 0 ||
			strncmp(opts.connection, "wss://", 6) == 0))
	{
		if (opts.insecure)
			ssl_opts.verify = 0;
		else
			ssl_opts.verify = 1;
		ssl_opts.CApath = opts.capath;	
		ssl_opts.keyStore = opts.cert;
		ssl_opts.trustStore = opts.cafile;
		ssl_opts.privateKey = opts.key;
		ssl_opts.privateKeyPassword = opts.keypass;
		ssl_opts.enabledCipherSuites = opts.ciphers;
		ssl_opts.ssl_error_cb = onSSLError;
		ssl_opts.ssl_error_context = client;
		ssl_opts.ssl_psk_cb = onPSKAuth;
		ssl_opts.ssl_psk_context = &opts;
		conn_opts.ssl = &ssl_opts;
	}

	#endif
	connected = 0;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		fprintf(stderr, "Failed to start connect, return code %s\n", MQTTAsync_strerror(rc));
		exit(EXIT_FAILURE);
	}
}


int mypublish(MQTTAsync client, int datalen, char* data)
{
	int rc;

	printf("Publishing data of length %d\n", datalen);

	rc = MQTTAsync_send(client, opts.topic, datalen, data, opts.qos, opts.retained, &pub_opts);
	if (opts.verbose && rc != MQTTASYNC_SUCCESS && !opts.quiet)
		fprintf(stderr, "Error from MQTTAsync_send: %s\n", MQTTAsync_strerror(rc));

	return rc;
}


void trace_callback(enum MQTTASYNC_TRACE_LEVELS level, char* message)
{
	fprintf(stderr, "Trace : %d, %s\n", level, message);
}

MQTTAsync client;
int mqtt_init()
{
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	MQTTAsync_createOptions create_opts = MQTTAsync_createOptions_initializer;
	char* buffer = NULL;
	char* url = NULL;
	int url_allocated = 0;
	int rc = 0;
	const char* version = NULL;
	const char* program_name = "paho_c_pub";
	MQTTAsync_nameValue* infos = MQTTAsync_getVersionInfo();
    struct sigaction sa;

	url = "192.168.31.201:1883";

	MQTTAsync_setTraceCallback(trace_callback);
	MQTTAsync_setTraceLevel(5);

	create_opts.sendWhileDisconnected = 1;
	create_opts.MQTTVersion = MQTTVERSION_5;
	rc = MQTTAsync_createWithOptions(&client, url, opts.clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL, &create_opts);
	if (rc != MQTTASYNC_SUCCESS)
	{
		fprintf(stderr, "Failed to create client, return code: %s\n", MQTTAsync_strerror(rc));
		return EXIT_FAILURE;
	}
#if 0
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = cfinish;
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
#endif
	rc = MQTTAsync_setCallbacks(client, client, NULL, messageArrived, NULL);
	if (rc != MQTTASYNC_SUCCESS)
	{
		fprintf(stderr, "Failed to set callbacks, return code: %s\n", MQTTAsync_strerror(rc));
		return EXIT_FAILURE;
	}

	pub_opts.onSuccess5 = onPublish5;
	pub_opts.onFailure5 = onPublishFailure5;

	property.identifier = MQTTPROPERTY_CODE_MESSAGE_EXPIRY_INTERVAL;
	property.value.integer4 = opts.message_expiry;
	MQTTProperties_add(&props, &property);

	if (0)
	{
		property.identifier = MQTTPROPERTY_CODE_USER_PROPERTY;
		property.value.data.data = opts.user_property.name;
		property.value.data.len = (int)strlen(opts.user_property.name);
		property.value.value.data = opts.user_property.value;
		property.value.value.len = (int)strlen(opts.user_property.value);
		MQTTProperties_add(&props, &property);
	}
	pub_opts.properties = props;

	myconnect(client);

#if 0
	while (!toStop)
	{
		int data_len = 0;
		int delim_len = 0;

		if (opts.stdin_lines)
		{
			buffer = malloc(opts.maxdatalen);

			delim_len = (int)strlen(opts.delimiter);
			do
			{
				buffer[data_len++] = getchar();
				if (data_len > delim_len)
				{
					if (strncmp(opts.delimiter, &buffer[data_len - delim_len], delim_len) == 0)
						break;
				}
			} while (data_len < opts.maxdatalen);

			rc = mypublish(client, data_len, buffer);
		}
		else
			mysleep(100);
	}

	if (opts.message == 0 && opts.null_message == 0 && opts.filename == 0)
		free(buffer);

	if (opts.MQTTVersion >= MQTTVERSION_5)
		disc_opts.onSuccess5 = onDisconnect5;
	else
		disc_opts.onSuccess = onDisconnect;
	if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		if (!opts.quiet)
			fprintf(stderr, "Failed to start disconnect, return code: %s\n", MQTTAsync_strerror(rc));
		exit(EXIT_FAILURE);
	}

	while (!disconnected)
		mysleep(100);

	MQTTAsync_destroy(&client);

	if (url_allocated)
		free(url);
#endif

	return EXIT_SUCCESS;
}
 

 void mqtt_guard_on()
 {
	//mypublish(client, 2, "ON");
	//MQTTAsync_send(client, "/livingroom/guard/cat/state/sw", 2, "ON", opts.qos, opts.retained, &pub_opts);
 }

 void mqtt_guard_off()
 {
	//mypublish(client, 3, "OFF");
	//MQTTAsync_send(client, "/livingroom/guard/cat/state/sw", 3, "OFF", opts.qos, opts.retained, &pub_opts);
 }

 void mqtt_cat_locationreport(int sX, int sY, int eX, int eY)
 {
	char data_buff[64];
	memset(data_buff, 0, 64);
	sprintf(data_buff,"sX:%d sY:%d eX:%d eY:%d", sX, sY, eX, eY);
	MQTTAsync_send(client, "/livingroom/guard/cat/state/ps", strlen(data_buff), data_buff, opts.qos, opts.retained, &pub_opts);
 }
/*
炮台范围：
X: 40-140
y: 50-75
*/
static int shake_val = 5;
void mqtt_guard_ps_set(int sX, int sY, int eX, int eY)
{
	float x_angle,y_angle;
	float FOV = 98.3;
	shake_val = -1 * shake_val;
	x_angle = ((float)sX / 19.2) + 40 + shake_val; //((float)sX * 100 / 1920) + 40
	y_angle = ((float)sY / 43.2) + 50; //((float)sY * 25 / 1080)  + 50
	

	char data_buff[64];
	memset(data_buff, 0, 64);
	sprintf(data_buff,"%d,%d", (int)x_angle, (int)y_angle);
	MQTTAsync_send(client, "battery1/coordinate", strlen(data_buff), data_buff, opts.qos, opts.retained, &pub_opts);
}

#ifdef __cplusplus
}
#endif