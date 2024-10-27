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
 *    Ian Craggs - fix for bug 413429 - connectionLost not called
 *    Guilherme Maciel Ferreira - add keep alive option
 *    Ian Craggs - add full capability
 *******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
#include "MQTTAsync.h"
#include "MQTTClientPersistence.h"
#include "pubsub_c_opts.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>


#if defined(_WIN32)
#include <windows.h>
#define sleep Sleep
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#if defined(_WRS_KERNEL)
#include <OsWrapper.h>
#endif

volatile int finished = 0;
int subscribed = 0;
int disconnected = 0;
int Teasing_sw = 1;


int Get_TeasingSW()
{
	return Teasing_sw;
}

void mysleep_s(int ms)
{
	#if defined(_WIN32)
		Sleep(ms);
	#else
		usleep(ms * 1000);
	#endif
}

void cfinish_s(int sig)
{
	signal(SIGINT, NULL);
	finished = 1;
}


struct pubsub_opts opts_s =
{
	0, 0, 0, 0, "\n", 100,  	/* debug/app options */
	NULL, NULL, 1, 0, 0, /* message options */
	MQTTVERSION_DEFAULT, NULL, "paho-c-sub", 0, 0, NULL, NULL, "localhost", "1883", NULL, 10, /* MQTT options */
	NULL, NULL, 0, 0, /* will options */
	0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* TLS options */
	0, {NULL, NULL}, /* MQTT V5 options */
	NULL, NULL, /* HTTP and HTTPS proxies */
};

int messageArrived_s(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
	size_t delimlen = 0;
	if(message->payloadlen < 3)
	{
		 goto BREAK;
	}

	if(((char*)message->payload)[1] == '1')
	{
		Teasing_sw = 1;
		printf("Teasing_sw on\r\n");
	}
	else
	{
		Teasing_sw = 0;
	}

	printf("mqtt msg:");

	if (opts_s.verbose)
		printf("%d %s\t", message->payloadlen, topicName);
	if (opts_s.delimiter)
		delimlen = strlen(opts_s.delimiter);
	if (opts_s.delimiter == NULL || (message->payloadlen > delimlen &&
		strncmp(opts_s.delimiter, &((char*)message->payload)[message->payloadlen - delimlen], delimlen) == 0))
		printf("%.*s", message->payloadlen, (char*)message->payload);
	else
		printf("%.*s%s", message->payloadlen, (char*)message->payload, opts_s.delimiter);
	/*
	if (message->struct_version == 1 && opts_s.verbose)
		logProperties(&message->properties);
	*/
BREAK:
	fflush(stdout);
	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);
	return 1;
}


void onDisconnect_s(void* context, MQTTAsync_successData* response)
{
	disconnected = 1;
}


void onSubscribe5_s(void* context, MQTTAsync_successData5* response)
{
	subscribed = 1;
}

void onSubscribe_s(void* context, MQTTAsync_successData* response)
{
	subscribed = 1;
}


void onSubscribeFailure5_s(void* context, MQTTAsync_failureData5* response)
{
	if (!opts_s.quiet)
		fprintf(stderr, "Subscribe failed, rc %s reason code %s\n",
				MQTTAsync_strerror(response->code),
				MQTTReasonCode_toString(response->reasonCode));
	finished = 1;
}


void onSubscribeFailure_s(void* context, MQTTAsync_failureData* response)
{
	if (!opts_s.quiet)
		fprintf(stderr, "Subscribe failed, rc %s\n",
			MQTTAsync_strerror(response->code));
	finished = 1;
}


void onConnectFailure5_s(void* context, MQTTAsync_failureData5* response)
{
	if (!opts_s.quiet)
		fprintf(stderr, "Connect failed, rc %s reason code %s\n",
			MQTTAsync_strerror(response->code),
			MQTTReasonCode_toString(response->reasonCode));
	finished = 1;
}


void onConnectFailure_s(void* context, MQTTAsync_failureData* response)
{
	if (!opts_s.quiet)
		fprintf(stderr, "Connect failed, rc %s\n", response ? MQTTAsync_strerror(response->code) : "none");
	finished = 1;
}


void onConnect5_s(void* context, MQTTAsync_successData5* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_callOptions copts = MQTTAsync_callOptions_initializer;
	int rc;

	if (opts_s.verbose)
		printf("Subscribing to topic %s with client %s at QoS %d\n", opts_s.topic, opts_s.clientid, opts_s.qos);

	copts.onSuccess5 = onSubscribe5_s;
	copts.onFailure5 = onSubscribeFailure5_s;
	copts.context = client;
	if ((rc = MQTTAsync_subscribe(client, opts_s.topic, opts_s.qos, &copts)) != MQTTASYNC_SUCCESS)
	{
		if (!opts_s.quiet)
			fprintf(stderr, "Failed to start subscribe, return code %s\n", MQTTAsync_strerror(rc));
		finished = 1;
	}
}


void onConnect_s(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions ropts = MQTTAsync_responseOptions_initializer;
	int rc;

	if (opts_s.verbose)
		printf("Subscribing to topic %s with client %s at QoS %d\n", opts_s.topic, opts_s.clientid, opts_s.qos);

	ropts.onSuccess = onSubscribe_s;
	ropts.onFailure = onSubscribeFailure_s;
	ropts.context = client;
	if ((rc = MQTTAsync_subscribe(client, opts_s.topic, opts_s.qos, &ropts)) != MQTTASYNC_SUCCESS)
	{
		if (!opts_s.quiet)
			fprintf(stderr, "Failed to start subscribe, return code %s\n", MQTTAsync_strerror(rc));
		finished = 1;
	}
}

MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;


void trace_callback_s(enum MQTTASYNC_TRACE_LEVELS level, char* message)
{
	fprintf(stderr, "Trace : %d, %s\n", level, message);
}


int mqttsub_init()
{
	MQTTAsync client;
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	MQTTAsync_createOptions create_opts = MQTTAsync_createOptions_initializer;
	MQTTAsync_willOptions will_opts = MQTTAsync_willOptions_initializer;
	MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
	int rc = 0;
	char* url = NULL;
	const char* version = NULL;
	const char* program_name = "paho_c_sub";
	MQTTAsync_nameValue* infos = MQTTAsync_getVersionInfo();
	
#if !defined(_WIN32)
    struct sigaction sa;
#endif

	opts_s.topic = "/livingroom/battery1/sw";
	url = "192.168.31.201:1883";

	opts_s.MQTTVersion = MQTTVERSION_5;
	create_opts.MQTTVersion = MQTTVERSION_5;
	rc = MQTTAsync_createWithOptions(&client, url, opts_s.clientid, MQTTCLIENT_PERSISTENCE_NONE,
			NULL, &create_opts);
	if (rc != MQTTASYNC_SUCCESS)
	{
		if (!opts_s.quiet)
			fprintf(stderr, "Failed to create client, return code: %s\n", MQTTAsync_strerror(rc));
		exit(EXIT_FAILURE);
	}

	rc = MQTTAsync_setCallbacks(client, client, NULL, messageArrived_s, NULL);
	if (rc != MQTTASYNC_SUCCESS)
	{
		if (!opts_s.quiet)
			fprintf(stderr, "Failed to set callbacks, return code: %s\n", MQTTAsync_strerror(rc));
		exit(EXIT_FAILURE);
	}

#if defined(_WIN32)
	signal(SIGINT, cfinish);
	signal(SIGTERM, cfinish);
#else
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = cfinish_s;
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
#endif

	MQTTAsync_connectOptions conn_opts5 = MQTTAsync_connectOptions_initializer5;
	conn_opts = conn_opts5;
	conn_opts.onSuccess5 = onConnect5_s;
	conn_opts.onFailure5 = onConnectFailure5_s;
	conn_opts.cleanstart = 1;
	conn_opts.keepAliveInterval = 100;
	conn_opts.username = "admin";
	conn_opts.password = "HMZB1527733";
	conn_opts.MQTTVersion = MQTTVERSION_5;
	conn_opts.context = client;
	conn_opts.automaticReconnect = 1;
	conn_opts.httpProxy = opts_s.http_proxy;
	conn_opts.httpsProxy = opts_s.https_proxy;

	if (opts_s.will_topic) 	/* will options */
	{
		will_opts.message = opts_s.will_payload;
		will_opts.topicName = opts_s.will_topic;
		will_opts.qos = opts_s.will_qos;
		will_opts.retained = opts_s.will_retain;
		conn_opts.will = &will_opts;
	}


	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		if (!opts_s.quiet)
			fprintf(stderr, "Failed to start connect, return code %s\n", MQTTAsync_strerror(rc));
		exit(EXIT_FAILURE);
	}
	return EXIT_SUCCESS;

}
#ifdef __cplusplus
}
#endif