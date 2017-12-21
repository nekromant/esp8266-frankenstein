#ifndef SENSORLOGGER_H
#define SENSORLOGGER_H

struct slogger_data_type {
	const char *	type;
	const char *	description;
	const char *	unit;
};

struct slogger_instance {
	char *				deviceId;
	char *				deviceName;
	char *				deviceType;
	char *				deviceGroup;
	char *				deviceParentGroup;
	char *				nextCloudUrl;
	char *				userName;
	char *				password;
	struct slogger_data_type	deviceDataTypes[];
};

struct slogger_http_request {
	char *	data;
	char *	url;
	char *	headers;
};




#endif /* end of include guard: SENSORLOGGER_H */
