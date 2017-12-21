#ifndef SENSORLOGGER_H
#define SENSORLOGGER_H

struct slogger_data_type {
	const char *	type;
	const char *	description;
	const char *	unit;
	int		dataTypeId;
	double		(*get_current_value)(struct slogger_data_type *current);
	void *		user_arg;
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
	void *	userdata;
};


struct slogger_http_request *slogger_instance_rq_post(struct slogger_instance *inst);
struct slogger_http_request *slogger_instance_rq_register(struct slogger_instance *inst);




#endif /* end of include guard: SENSORLOGGER_H */
