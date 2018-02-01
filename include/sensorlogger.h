#ifndef SENSORLOGGER_H
#define SENSORLOGGER_H

struct slogger_data_type {
	const char *	type;
	const char *	description;
	const char *	unit;
	int				dataTypeId;
	double			(*get_current_value)(struct slogger_data_type *current);
	void *			user_arg;
	double			current_value;
	struct slogger_data_type *next;
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
	struct slogger_data_type	*deviceDataTypes;
};

struct slogger_http_request {
	char *	data;
	char *	url;
	char *	headers;
	void *	userdata;
};


/**
 * Registers a data type structure within slogger intance.
 *
 * @param inst slogger instance
 * @param new  the new structure to register
 */
void sensorlogger_instance_register_data_type(struct slogger_instance *inst, struct slogger_data_type *new);

/**
 * Generates a http request for registering this device within nextcloud.
 * User should take care to deliver this request with platform-specific network API
 *
 * @param  inst instance to register
 * @return    the http request structure
 */
struct slogger_http_request *slogger_instance_rq_register(struct slogger_instance *inst);

/**
 * Generates a http request for obtaining data type ids from the server.
 * You should save the server's response and feed it to slogger_instance_populate_data_type_ids()
 *
 * @param  inst [description]
 * @return      [description]
 */
struct slogger_http_request *slogger_instance_rq_get_data_types(struct slogger_instance *inst);

/**
 * Parses json and populates the data type ids. This is required before posting any data
 *
 * @param inst [description]
 * @param json [description]
 */
void slogger_instance_populate_data_type_ids(struct slogger_instance *inst, char *json);

/**
 * Generates a http request to post sensor data to the server
 *
 * @param  inst [description]
 * @return      [description]
 */
struct slogger_http_request *slogger_instance_rq_post(struct slogger_instance *inst);

/**
 * Used to free an http request structure. Don't forget to do it or memory will
 * leak
 *
 * @param rq [description]
 */
void slogger_http_request_release(struct slogger_http_request *rq);


/**
 * Tells if all DataTypeIds have been filled and the instance is
 * ready to accept data posts
 * @param  inst [description]
 * @return      [description]
 */
int slogger_instance_is_ready(struct slogger_instance *inst);

void slogger_instance_set_current_value(struct slogger_instance *inst,
					       char *			description,
					       char *			type,
					       char *			shortd,
					       double			value);

/**
 * Dumps current senslogger instance to stdout
 * @param inst [description]
 */
void slogger_instance_dump(struct slogger_instance *inst);

#endif /* end of include guard: SENSORLOGGER_H */
