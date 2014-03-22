
/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2009 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Ericsson AB
 *
 */

/**
 *   NTF subscriber command line program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <poll.h>

#include <saAis.h>
#include <saAmf.h>
#include <saClm.h>
#include <saNtf.h>
#include <ntfclient.h>

static int verbose;
static SaNtfHandleT ntfHandle;
static SaSelectionObjectT selObj;

/* Name of current testproxy (argv[0]) */
static char *progname;

/* Release code, major version, minor version */
static SaVersionT version = { 'A', 0x01, 0x01 };

/* help functions for printouts */
static void _exitIfFalse(const char *file, unsigned int line, int expression)
{				/* instead of assert */
	if (!expression) {
		fprintf(stderr, "[%s:%04u] EXIT_FAILURE\n", file, line);
		exit(EXIT_FAILURE);
	}
}
#define EXIT_IF_FALSE(expr) _exitIfFalse(__FILE__, __LINE__, expr)

static struct s_filters_T {
	int all; 
	int alarm;
 	int obj_cr_del;
	int att_ch;
	int st_ch;
	int sec_al;
} used_filters = {1,0,0,0,0,0};

static const char *sa_probable_cause_list[] = {
	"SA_NTF_ADAPTER_ERROR",
	"SA_NTF_APPLICATION_SUBSYSTEM_FAILURE",
	"SA_NTF_BANDWIDTH_REDUCED",
	"SA_NTF_CALL_ESTABLISHMENT_ERROR",
	"SA_NTF_COMMUNICATIONS_PROTOCOL_ERROR",
	"SA_NTF_COMMUNICATIONS_SUBSYSTEM_FAILURE",
	"SA_NTF_CONFIGURATION_OR_CUSTOMIZATION_ERROR",
	"SA_NTF_CONGESTION",
	"SA_NTF_CORRUPT_DATA",
	"SA_NTF_CPU_CYCLES_LIMIT_EXCEEDED",
	"SA_NTF_DATASET_OR_MODEM_ERROR",
	"SA_NTF_DEGRADED_SIGNAL",
	"SA_NTF_D_T_E",
	"SA_NTF_ENCLOSURE_DOOR_OPEN",
	"SA_NTF_EQUIPMENT_MALFUNCTION",
	"SA_NTF_EXCESSIVE_VIBRATION",
	"SA_NTF_FILE_ERROR",
	"SA_NTF_FIRE_DETECTED",
	"SA_NTF_FLOOD_DETECTED",
	"SA_NTF_FRAMING_ERROR",
	"SA_NTF_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM",
	"SA_NTF_HUMIDITY_UNACCEPTABLE",
	"SA_NTF_INPUT_OUTPUT_DEVICE_ERROR",
	"SA_NTF_INPUT_DEVICE_ERROR",
	"SA_NTF_L_A_N_ERROR",
	"SA_NTF_LEAK_DETECTED",
	"SA_NTF_LOCAL_NODE_TRANSMISSION_ERROR",
	"SA_NTF_LOSS_OF_FRAME",
	"SA_NTF_LOSS_OF_SIGNAL",
	"SA_NTF_MATERIAL_SUPPLY_EXHAUSTED",
	"SA_NTF_MULTIPLEXER_PROBLEM",
	"SA_NTF_OUT_OF_MEMORY",
	"SA_NTF_OUTPUT_DEVICE_ERROR",
	"SA_NTF_PERFORMANCE_DEGRADED",
	"SA_NTF_POWER_PROBLEM",
	"SA_NTF_PRESSURE_UNACCEPTABLE",
	"SA_NTF_PROCESSOR_PROBLEM",
	"SA_NTF_PUMP_FAILURE",
	"SA_NTF_QUEUE_SIZE_EXCEEDED",
	"SA_NTF_RECEIVE_FAILURE",
	"SA_NTF_RECEIVER_FAILURE",
	"SA_NTF_REMOTE_NODE_TRANSMISSION_ERROR",
	"SA_NTF_RESOURCE_AT_OR_NEARING_CAPACITY",
	"SA_NTF_RESPONSE_TIME_EXCESSIVE",
	"SA_NTF_RETRANSMISSION_RATE_EXCESSIVE",
	"SA_NTF_SOFTWARE_ERROR",
	"SA_NTF_SOFTWARE_PROGRAM_ABNORMALLY_TERMINATED",
	"SA_NTF_SOFTWARE_PROGRAM_ERROR",
	"SA_NTF_STORAGE_CAPACITY_PROBLEM",
	"SA_NTF_TEMPERATURE_UNACCEPTABLE",
	"SA_NTF_THRESHOLD_CROSSED",
	"SA_NTF_TIMING_PROBLEM",
	"SA_NTF_TOXIC_LEAK_DETECTED",
	"SA_NTF_TRANSMIT_FAILURE",
	"SA_NTF_TRANSMITTER_FAILURE",
	"SA_NTF_UNDERLYING_RESOURCE_UNAVAILABLE",
	"SA_NTF_VERSION_MISMATCH",
	"SA_NTF_AUTHENTICATION_FAILURE",
	"SA_NTF_BREACH_OF_CONFIDENTIALITY",
	"SA_NTF_CABLE_TAMPER",
	"SA_NTF_DELAYED_INFORMATION",
	"SA_NTF_DENIAL_OF_SERVICE",
	"SA_NTF_DUPLICATE_INFORMATION",
	"SA_NTF_INFORMATION_MISSING",
	"SA_NTF_INFORMATION_MODIFICATION_DETECTED",
	"SA_NTF_INFORMATION_OUT_OF_SEQUENCE",
	"SA_NTF_INTRUSION_DETECTION",
	"SA_NTF_KEY_EXPIRED",
	"SA_NTF_NON_REPUDIATION_FAILURE",
	"SA_NTF_OUT_OF_HOURS_ACTIVITY",
	"SA_NTF_OUT_OF_SERVICE",
	"SA_NTF_PROCEDURAL_ERROR",
	"SA_NTF_UNAUTHORIZED_ACCESS_ATTEMPT",
	"SA_NTF_UNEXPECTED_INFORMATION",
	"SA_NTF_UNSPECIFIED_REASON",
};

static const char *sa_severity_list[] = {
	"SA_NTF_SEVERITY_CLEARED",
	"SA_NTF_SEVERITY_INDETERMINATE",
	"SA_NTF_SEVERITY_WARNING",
	"SA_NTF_SEVERITY_MINOR",
	"SA_NTF_SEVERITY_MAJOR",
	"SA_NTF_SEVERITY_CRITICAL",
};

static const char *sa_alarm_event_type_list[] = {
	"SA_NTF_ALARM_NOTIFICATIONS_START",
	"SA_NTF_ALARM_COMMUNICATION",
	"SA_NTF_ALARM_QOS",
	"SA_NTF_ALARM_PROCESSING",
	"SA_NTF_ALARM_EQUIPMENT",
	"SA_NTF_ALARM_ENVIRONMENT",
};

static const char *sa_state_change_event_type_list[] = {
	"SA_NTF_STATE_CHANGE_NOTIFICATIONS_START",
	"SA_NTF_OBJECT_STATE_CHANGE",
};

static const char *sa_object_create_delete_event_type_list[] = {
	"SA_NTF_OBJECT_NOTIFICATIONS_START",
	"SA_NTF_OBJECT_CREATION",
	"SA_NTF_OBJECT_DELETION",
};

static const char *sa_attribute_change_event_type_list[] = {
	"SA_NTF_ATTRIBUTE_NOTIFICATIONS_START",
	"SA_NTF_ATTRIBUTE_ADDED",
	"SA_NTF_ATTRIBUTE_REMOVED",
	"SA_NTF_ATTRIBUTE_CHANGED",
	"SA_NTF_ATTRIBUTE_RESET",
};

static const char *sa_security_alarm_event_type_list[] = {
	"SA_NTF_SECURITY_ALARM_NOTIFICATIONS_START",
	"SA_NTF_INTEGRITY_VIOLATION",
	"SA_NTF_OPERATION_VIOLATION",
	"SA_NTF_PHYSICAL_VIOLATION",
	"SA_NTF_SECURITY_SERVICE_VIOLATION",
	"SA_NTF_TIME_VIOLATION",
};

static const char *sa_miscellaneous_event_type_list[] = {
    "SA_NTF_MISCELLANEOUS_NOTIFICATIONS_START",
    "SA_NTF_APPLICATION_EVENT",
    "SA_NTF_ADMIN_OPERATION_START",
    "SA_NTF_ADMIN_OPERATION_END",
    "SA_NTF_CONFIG_UPDATE_START",
    "SA_NTF_CONFIG_UPDATE_END",
    "SA_NTF_ERROR_REPORT",
    "SA_NTF_ERROR_CLEAR",
    "SA_NTF_HPI_EVENT_RESOURCE",
    "SA_NTF_HPI_EVENT_SENSOR",
    "SA_NTF_HPI_EVENT_WATCHDOG",
    "SA_NTF_HPI_EVENT_DIMI",
    "SA_NTF_HPI_EVENT_FUMI",
    "SA_NTF_HPI_EVENT_OTHER",
};

static const char *sa_source_indicator_list[] = {
	"SA_NTF_OBJECT_OPERATION", /* previous error in saNtf.h */
	"SA_NTF_OBJECT_OPERATION",
	"SA_NTF_MANAGEMENT_OPERATION",
	"SA_NTF_UNKNOWN_OPERATION",
};

static const char *sa_services_list[] = {
	"",
	"SA_SVC_HPI",
	"SA_SVC_AMF",
	"SA_SVC_CLM",
	"SA_SVC_CKPT",
	"SA_SVC_EVT",
	"SA_SVC_MSG",
	"SA_SVC_LCK",
	"SA_SVC_IMM", 
	"SA_SCV_LOG",
	"SA_SVC_NTF",
	"SA_SVC_NAM",
	"SA_SVC_TMR",
	"SA_SVC_SMF",
	"SA_SVC_SEC",
	"SA_SVC_PLM"
};

static const char *sa_amf_state_list[] = {
	"",
	"SA_AMF_READINESS_STATE",
	"SA_AMF_HA_STATE",
	"SA_AMF_PRESENCE_STATE",
	"SA_AMF_OP_STATE",
	"SA_AMF_ADMIN_STATE",
	"SA_AMF_ASSIGNMENT_STATE",
	"SA_AMF_PROXY_STATUS",
	"SA_AMF_HA_READINESS_STATE"
};

static const char *sa_amf_pres_state_list[] = {
	"",
	"SA_AMF_PRESENCE_UNINSTANTIATED",
	"SA_AMF_PRESENCE_INSTANTIATING",
	"SA_AMF_PRESENCE_INSTANTIATED",
	"SA_AMF_PRESENCE_TERMINATING",
	"SA_AMF_PRESENCE_RESTARTING",
	"SA_AMF_PRESENCE_INSTANTIATION_FAILED",
	"SA_AMF_PRESENCE_TERMINATION_FAILED"
};

static const char *sa_amf_op_state_list[] = {
	"",
	"SA_AMF_OPERATIONAL_ENABLED",
	"SA_AMF_OPERATIONAL_DISABLED"
};

static const char *sa_amf_ha_state_list[] = {
	"",
	"SA_AMF_HA_ACTIVE",
	"SA_AMF_HA_STANDBY",
	"SA_AMF_HA_QUIESCED",
	"SA_AMF_HA_QUIESCING"
};

static const char *sa_amf_adm_state_list[] = {
	"",
	"SA_AMF_ADMIN_UNLOCKED",
	"SA_AMF_ADMIN_LOCKED",
	"SA_AMF_ADMIN_LOCKED_INSTANTIATION",
	"SA_AMF_ADMIN_SHUTTING_DOWN"
};

static const char *sa_amf_ass_state_list[] = {
	"",
	"SA_AMF_ASSIGNMENT_UNASSIGNED",
	"SA_AMF_ASSIGNMENT_FULLY_ASSIGNED",
	"SA_AMF_ASSIGNMENT_PARTIALLY_ASSIGNED"
};

static const char *sa_clm_state_list[] = {
	"",
	"SA_CLM_CLUSTER_CHANGE_STATUS",
	"SA_CLM_ADMIN_STATE"
};

static const char *sa_clm_change_state_list[] = {
	"",
	"SA_CLM_NODE_NO_CHANGE",
	"SA_CLM_NODE_JOINED",
	"SA_CLM_NODE_LEFT",
	"SA_CLM_NODE_RECONFIGURED"
};

static char *error_output(SaAisErrorT result)
{
	static char error_result[256];

	sprintf(error_result, "error: %u", result);
	return (error_result);
}

static void print_severity(SaNtfSeverityT input)
{
	EXIT_IF_FALSE(input >= SA_NTF_SEVERITY_CLEARED);
	EXIT_IF_FALSE(input <= SA_NTF_SEVERITY_CRITICAL);

	printf("perceivedSeverity = ");
	printf("%s\n", (char *)sa_severity_list[input]);
}

static void print_probable_cause(SaNtfProbableCauseT input)
{
	EXIT_IF_FALSE(input >= SA_NTF_ADAPTER_ERROR);
	EXIT_IF_FALSE(input <= SA_NTF_UNSPECIFIED_REASON);

	printf("probableCause = ");
	printf("%s\n", (char *)sa_probable_cause_list[input]);
}

static void print_event_type(SaNtfEventTypeT input, SaNtfNotificationTypeT notificationType)
{
	int listIndex;

	switch (notificationType) {
	case SA_NTF_TYPE_STATE_CHANGE:
		if (input >= (int)SA_NTF_MISCELLANEOUS_NOTIFICATIONS_START) {
			listIndex = (int)input - (int)SA_NTF_TYPE_MISCELLANEOUS;
			
			EXIT_IF_FALSE(input >= SA_NTF_MISCELLANEOUS_NOTIFICATIONS_START);
			EXIT_IF_FALSE(input <= SA_NTF_HPI_EVENT_OTHER);
			
			printf("%s\n", (char *)sa_miscellaneous_event_type_list[listIndex]);
		} else if (input >= (int)SA_NTF_STATE_CHANGE_NOTIFICATIONS_START) {
			listIndex = (int)input - (int)SA_NTF_TYPE_STATE_CHANGE;
			  
			EXIT_IF_FALSE(input >= SA_NTF_STATE_CHANGE_NOTIFICATIONS_START);
			EXIT_IF_FALSE(input <= SA_NTF_OBJECT_STATE_CHANGE);
			
			printf("%s\n", (char *)sa_state_change_event_type_list[listIndex]);
		}
		break;

	case SA_NTF_TYPE_ALARM:
		if (input >= (int)SA_NTF_ALARM_NOTIFICATIONS_START) {
			listIndex = (int)input - (int)SA_NTF_TYPE_ALARM;

			EXIT_IF_FALSE(input >= SA_NTF_ALARM_NOTIFICATIONS_START);
			EXIT_IF_FALSE(input <= SA_NTF_ALARM_ENVIRONMENT);

			printf("%s\n", (char *)sa_alarm_event_type_list[listIndex]);
		}
		break;

	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		if (input >= (int)SA_NTF_OBJECT_NOTIFICATIONS_START) {
			listIndex = (int)input - (int)SA_NTF_TYPE_OBJECT_CREATE_DELETE;

			EXIT_IF_FALSE(input >= SA_NTF_OBJECT_NOTIFICATIONS_START);
			EXIT_IF_FALSE(input <= SA_NTF_OBJECT_DELETION);

			printf("%s\n", (char *)sa_object_create_delete_event_type_list[listIndex]);
		}
		break;

	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		if (input >= (int)SA_NTF_ATTRIBUTE_NOTIFICATIONS_START) {
			listIndex = (int)input - (int)SA_NTF_TYPE_ATTRIBUTE_CHANGE;

			EXIT_IF_FALSE(input >= SA_NTF_ATTRIBUTE_NOTIFICATIONS_START);
			EXIT_IF_FALSE(input <= SA_NTF_ATTRIBUTE_RESET);

			printf("%s\n", (char *)sa_attribute_change_event_type_list[listIndex]);
		}
		break;

	case SA_NTF_TYPE_SECURITY_ALARM:
		if (input >= (int)SA_NTF_SECURITY_ALARM_NOTIFICATIONS_START) {
			listIndex = (int)input - (int)SA_NTF_TYPE_SECURITY_ALARM;

			EXIT_IF_FALSE(input >= SA_NTF_SECURITY_ALARM_NOTIFICATIONS_START);
			EXIT_IF_FALSE(input <= SA_NTF_TIME_VIOLATION);

			printf("%s\n", (char *)sa_security_alarm_event_type_list[listIndex]);
		}
		break;

	default:
		printf("Unknown Notification Type!!");
		exit(1);
		break;
	}
}

static void print_change_states(SaNtfClassIdT *notificationClassId, SaNtfStateChangeT *input)
{
	if ((notificationClassId->vendorId == SA_NTF_VENDOR_ID_SAF) &&
		(notificationClassId->majorId == SA_SVC_AMF)) {

		assert(SA_AMF_READINESS_STATE <= input->stateId && input->stateId <= SA_AMF_HA_READINESS_STATE);
		printf("State ID = %s\n", sa_amf_state_list[input->stateId]);

		switch (input->stateId) {
		case SA_AMF_HA_STATE:
			if (input->oldStatePresent)
				printf("Old State: %s\n", sa_amf_ha_state_list[input->oldState]);
			printf("New State: %s\n", sa_amf_ha_state_list[input->newState]);
			break;
		case SA_AMF_PRESENCE_STATE:
			if (input->oldStatePresent)
				printf("Old State: %s\n", sa_amf_pres_state_list[input->oldState]);
			printf("New State: %s\n", sa_amf_pres_state_list[input->newState]);
			break;
		case SA_AMF_OP_STATE:
			if (input->oldStatePresent)
				printf("Old State: %s\n", sa_amf_op_state_list[input->oldState]);
			printf("New State: %s\n", sa_amf_op_state_list[input->newState]);
			break;
		case SA_AMF_ADMIN_STATE:
			if (input->oldStatePresent)
				printf("Old State: %s\n", sa_amf_adm_state_list[input->oldState]);
			printf("New State: %s\n", sa_amf_adm_state_list[input->newState]);
			break;
		case SA_AMF_ASSIGNMENT_STATE:
			if (input->oldStatePresent)
				printf("Old State: %s\n", sa_amf_ass_state_list[input->oldState]);
			printf("New State: %s\n", sa_amf_ass_state_list[input->newState]);
			break;
		default:
			if (input->oldStatePresent)
				printf("Old State: %u\n", input->oldState);
			printf("New State: %u\n", input->newState);
			break;
		}
	} else if ((notificationClassId->vendorId == SA_NTF_VENDOR_ID_SAF) &&
		(notificationClassId->majorId == SA_SVC_CLM)) {

		assert(SA_AMF_READINESS_STATE <= input->stateId && input->stateId <= SA_AMF_HA_READINESS_STATE);
		printf("State ID = %s\n", sa_clm_state_list[input->stateId]);

		switch (input->stateId) {
		case SA_CLM_CLUSTER_CHANGE_STATUS:
			if (input->oldStatePresent)
				printf("Old State: %s\n", sa_clm_change_state_list[input->oldState]);
			printf("New State: %s\n", sa_clm_change_state_list[input->newState]);
			break;
		default:
			if (input->oldStatePresent)
				printf("Old State: %u\n", input->oldState);
			printf("New State: %u\n", input->newState);
			break;
		}
	} else {
		printf("- State ID: %u -\n", input->stateId);

		if (input->oldStatePresent == SA_TRUE) {
			if (verbose)
				printf("Old State Present: Yes\n");
			printf("	Old State: %u\n", input->oldState);
		} else if (verbose)
			printf("Old	State Present: No\n");
	
		printf("New State: %u\n", input->newState);
	}
}

static void print_object_attributes(SaNtfAttributeT *input)
{
	printf("- Attribute ID: %d -\n", (int)input->attributeId);
	printf("Attribute Type: %d\n", (int)input->attributeType);
	printf("Attribute Value: %d\n", (int)input->attributeValue.int32Val);
}

static void print_changed_attributes(SaNtfAttributeChangeT *input)
{
	printf("- Attribute ID: %d -\n", input->attributeId);

	printf("Attribute Type: %d\n", input->attributeType);
	if (input->oldAttributePresent == SA_TRUE) {
		printf("Old Attribute Present: Yes\n");
		printf("Old Attribute: %d\n", input->oldAttributeValue.int32Val);
	} else {
		printf("Old Attribute Present: No\n");
	}
	printf("New Attribute Value: %d\n", input->newAttributeValue.int32Val);
}

static void print_security_alarm_types(SaNtfSecurityAlarmNotificationT *input)
{
	printf("Security Alarm Detector Type: %d\n", input->securityAlarmDetector->valueType);
	printf("Security Alarm Detector Value: %d\n", input->securityAlarmDetector->value.int32Val);

	printf("Service User Type: %d\n", input->serviceUser->valueType);
	printf("Service User Value: %d\n", input->serviceUser->value.int32Val);

	printf("Service Provider Type: %d\n", input->serviceProvider->valueType);
	printf("Service Provider Value: %d\n", input->serviceProvider->value.int32Val);
}

static void print_source_indicator(SaNtfSourceIndicatorT input)
{
	EXIT_IF_FALSE(input >= SA_NTF_OBJECT_OPERATION);
	EXIT_IF_FALSE(input <= SA_NTF_UNKNOWN_OPERATION);

	printf("sourceIndicator = ");
	printf("%s\n", (char *)sa_source_indicator_list[input]);
}

/* end help functions for printouts */

static void print_header(const SaNtfNotificationHeaderT *notificationHeader,
			 SaNtfSubscriptionIdT subscriptionId, SaNtfNotificationTypeT notificationType)
{
	char tmpObj[SA_MAX_NAME_LENGTH + 1];


	if (verbose) {
		printf("notificationID = %d\n", (int)*(notificationHeader->notificationId));
		printf("subscriptionId = %u\n", (unsigned int)subscriptionId);
	}

	/* Event type */
	printf("eventType = ");
	print_event_type(*notificationHeader->eventType, notificationType);

	if (verbose)
		printf("notificationObject.length = %u\n", notificationHeader->notificationObject->length);

	strncpy(tmpObj,
		(char *)notificationHeader->notificationObject->value, notificationHeader->notificationObject->length);
	tmpObj[notificationHeader->notificationObject->length] = '\0';
	printf("notificationObject = \"%s\"\n", tmpObj);

	strncpy(tmpObj,
		(char *)notificationHeader->notifyingObject->value, notificationHeader->notifyingObject->length);
	tmpObj[notificationHeader->notifyingObject->length] = '\0';

	if (verbose)
		printf("notifyingObject.length = %u\n", notificationHeader->notifyingObject->length);

	printf("notifyingObject = \"%s\"\n", tmpObj);

	/* Notification Class ID in compact dot notation */
	if (notificationHeader->notificationClassId->vendorId == SA_NTF_VENDOR_ID_SAF) {
		printf("notificationClassId = SA_NTF_VENDOR_ID_SAF.%s.%u (0x%x)\n",
			   sa_services_list[notificationHeader->notificationClassId->majorId],
			   notificationHeader->notificationClassId->minorId,
			   notificationHeader->notificationClassId->minorId);
	}
	else {
		printf("notificationClassId = %u.%u.%u (0x%x)\n",
			   notificationHeader->notificationClassId->vendorId,
			   notificationHeader->notificationClassId->majorId,
			   notificationHeader->notificationClassId->minorId,
			   notificationHeader->notificationClassId->minorId);
	}

	if (verbose)
		printf("eventTime = %lld\n", *notificationHeader->eventTime);

	if (notificationHeader->lengthAdditionalText > 0)
		printf("additionalText = \"%s\"\n", notificationHeader->additionalText);
}

static void print_additional_info(SaNtfNotificationHandleT notificationHandle,
				  const SaNtfNotificationHeaderT *notificationHeader)
{
	if (notificationHeader->additionalInfo != NULL) {
		switch (notificationHeader->additionalInfo[0].infoType) {
		case SA_NTF_VALUE_LDAP_NAME: {
			SaNameT *dataPtr;
			SaUint16T dataSize;
			SaAisErrorT rc;

			rc = saNtfPtrValGet(notificationHandle,
						   &notificationHeader->additionalInfo[0].infoValue,
						   (void **)&dataPtr, &dataSize);
			if (rc == SA_AIS_OK) {
				printf("additionalInfo = \"%s\"\n", dataPtr->value);
			} else
				fprintf(stderr, "saNtfPtrValGet Error %d\n", rc);
			break;
		}
		default:
			printf("Unimplemented additionalInfo type\n");
			break;
		}
	}
}

static char *event_time(SaTimeT eventTime)
{
	time_t time_in_secs;
	static char time_str[32];

	time_in_secs = (eventTime / (SaTimeT)SA_TIME_ONE_SECOND);
	(void)strftime(time_str, sizeof(time_str), "%b %e %k:%M:%S", localtime(&time_in_secs));

	return time_str;
}

static void saNtfNotificationCallback(SaNtfSubscriptionIdT subscriptionId, const SaNtfNotificationsT *notification)
{
	SaInt32T i;
	SaNtfNotificationHandleT notificationHandle;
	const SaNtfNotificationHeaderT *notificationHeader;

	switch (notification->notificationType) {
	case SA_NTF_TYPE_ALARM:
		notificationHandle = notification->notification.alarmNotification.notificationHandle;
		notificationHeader = &notification->notification.alarmNotification.notificationHeader;
		printf("===  %s - Alarm  ===\n", event_time(*notificationHeader->eventTime));
		print_header(notificationHeader, subscriptionId, notification->notificationType);
		print_additional_info(notificationHandle, notificationHeader);
		print_probable_cause(*(notification->notification.alarmNotification.probableCause));
		print_severity(*(notification->notification.alarmNotification.perceivedSeverity));

		break;

	case SA_NTF_TYPE_STATE_CHANGE:
		notificationHandle = notification->notification.stateChangeNotification.notificationHandle;
		notificationHeader = &notification->notification.stateChangeNotification.notificationHeader;
		printf("===  %s - State Change  ===\n", event_time(*notificationHeader->eventTime));
		print_header(notificationHeader, subscriptionId, notification->notificationType);
		print_additional_info(notificationHandle, notificationHeader);
		print_source_indicator(*(notification->notification.stateChangeNotification.sourceIndicator));

		if (verbose)
			printf("Num of StateChanges: %d\n", notification->notification.stateChangeNotification.numStateChanges);

		/* Changed states */
		for (i = 0; i < notification->notification.stateChangeNotification.numStateChanges; i++) {
			print_change_states(notificationHeader->notificationClassId,
				&notification->notification.stateChangeNotification.changedStates[i]);
		}
		break;

	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		notificationHandle = notification->notification.objectCreateDeleteNotification.notificationHandle;
		notificationHeader = &notification->notification.objectCreateDeleteNotification.notificationHeader;
		printf("===  %s - Object Create/Delete  ===\n", event_time(*notificationHeader->eventTime));
		print_header(notificationHeader, subscriptionId, notification->notificationType);
		print_source_indicator(*(notification->notification.objectCreateDeleteNotification.sourceIndicator));
		printf("numAttributes: %d\n", notification->notification.objectCreateDeleteNotification.numAttributes);

		/* Object Attributes */
		for (i = 0; i < notification->notification.objectCreateDeleteNotification.numAttributes; i++) {
			print_object_attributes(&notification->notification.
						objectCreateDeleteNotification.objectAttributes[i]);
		}
		break;

	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		notificationHandle = notification->notification.attributeChangeNotification.notificationHandle;
		notificationHeader = &notification->notification.attributeChangeNotification.notificationHeader;
		printf("===  %s - Attribute Change  ===\n", event_time(*notificationHeader->eventTime));
		print_header(notificationHeader, subscriptionId, notification->notificationType);
		print_source_indicator(*(notification->notification.attributeChangeNotification.sourceIndicator));
		printf("numAttributes: %d\n", notification->notification.attributeChangeNotification.numAttributes);

		/* Changed Attributes */
		for (i = 0; i < notification->notification.attributeChangeNotification.numAttributes; i++) {
			print_changed_attributes(&notification->notification.
						 attributeChangeNotification.changedAttributes[i]);
		}
		break;

	case SA_NTF_TYPE_SECURITY_ALARM:
		notificationHandle = notification->notification.securityAlarmNotification.notificationHandle;
		notificationHeader = &notification->notification.securityAlarmNotification.notificationHeader;
		printf("===  %s - Security Alarm  ===\n", event_time(*notificationHeader->eventTime));
		print_header(notificationHeader, subscriptionId, notification->notificationType);
		print_probable_cause(*(notification->notification.securityAlarmNotification.probableCause));
		print_severity(*(notification->notification.securityAlarmNotification.severity));
		print_security_alarm_types((SaNtfSecurityAlarmNotificationT *)&notification->notification.
					   securityAlarmNotification);

		break;

	default:
		printf("unknown notification type %d", (int)notification->notificationType);
		break;
	}

	switch (notification->notificationType) {
	case SA_NTF_TYPE_ALARM:
		saNtfNotificationFree(notification->notification.alarmNotification.notificationHandle);
		break;
	case SA_NTF_TYPE_SECURITY_ALARM:
		saNtfNotificationFree(notification->notification.securityAlarmNotification.notificationHandle);
		break;
	case SA_NTF_TYPE_STATE_CHANGE:
		saNtfNotificationFree(notification->notification.stateChangeNotification.notificationHandle);
		break;
	case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
		saNtfNotificationFree(notification->notification.objectCreateDeleteNotification.notificationHandle);
		break;
	case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
		saNtfNotificationFree(notification->notification.attributeChangeNotification.notificationHandle);
		break;

	default:
		printf("wrong type");
		assert(0);
	}
	printf("\n");
}

static void saNtfNotificationDiscardedCallback(SaNtfSubscriptionIdT subscriptionId,
					       SaNtfNotificationTypeT notificationType,
					       SaUint32T numberDiscarded,
					       const SaNtfIdentifierT *discardedNotificationIdentifiers)
{
	unsigned int i = 0;

	printf("Discarded callback function  notificationType: %d\n\
                  subscriptionId  : %u \n\
                  numberDiscarded : %u\n", (int)notificationType, (unsigned int)subscriptionId, (unsigned int)numberDiscarded);
	for (i = 0; i < numberDiscarded; i++)
		printf("[%u]", (unsigned int)discardedNotificationIdentifiers[i]);

	printf("\n");
}

static SaNtfCallbacksT ntfCallbacks = {
	saNtfNotificationCallback,
	saNtfNotificationDiscardedCallback
};

static SaAisErrorT waitForNotifications(SaNtfHandleT myHandle, int selectionObject, int timeout_ms)
{
	SaAisErrorT error;
	int rv;
	struct pollfd fds[1];

	fds[0].fd = (int)selectionObject;
	fds[0].events = POLLIN;

	for (;;) {
		rv = poll(fds, 1, timeout_ms);

		if (rv == -1) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "poll FAILED: %s\n", strerror(errno));
			return SA_AIS_ERR_BAD_OPERATION;
		}

		if (rv == 0) {
			printf("poll timeout\n");
			return SA_AIS_OK;
		}

		do {
			error = saNtfDispatch(myHandle, SA_DISPATCH_ALL);
			if (SA_AIS_ERR_TRY_AGAIN == error)
				sleep(1);
		} while (SA_AIS_ERR_TRY_AGAIN == error);

		if (error != SA_AIS_OK)
			fprintf(stderr, "saNtfDispatch Error %d\n", error);
	}

	return error;
}

static void usage(void)
{
	printf("\nNAME\n");
	printf("\t%s - subscribe for notfifications\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s subscribe [-t timeout]\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is a SAF NTF client used to subscribe for all incoming notifications.\n", progname);
	printf("\nOPTIONS\n");
	printf("  -t or --timeout=TIME                      timeout (sec) waiting for notification\n");
	printf("  -a or --alarm                             subscribe for only alarm notifications\n");
	printf("  -o or --objectCreateDelete                subscribe for only objectCreateDelete notifications\n");
	printf("  -c or --attributeChange                   subscribe for only attributeChange notifications\n");
	printf("  -s or --stateChange                       subscribe for only stateChange notifications\n");
	printf("  -y or --securityAlarm                     subscribe for only securityAlarm notifications\n");
	printf("  -h or --help                              this help\n");
	printf("  -v or --verbose                           print even more\n");
	exit((int)SA_AIS_ERR_INVALID_PARAM);
}

static void freeNtfFilter(SaNtfNotificationFilterHandleT *fh_ptr)
{
	SaAisErrorT errorCode = SA_AIS_OK;
	if (*fh_ptr) {  
		errorCode = saNtfNotificationFilterFree(*fh_ptr);
		if (SA_AIS_OK != errorCode) {
			fprintf(stderr, "saNtfNotificationFilterFree failed - %s\n", error_output(errorCode));
			exit(EXIT_FAILURE);
		}
	}
}

/* Subscribe */
static SaAisErrorT subscribeForNotifications(const saNotificationFilterAllocationParamsT
					     *notificationFilterAllocationParams, SaNtfSubscriptionIdT subscriptionId)
{
	SaAisErrorT errorCode = SA_AIS_OK;
	SaNtfAlarmNotificationFilterT myAlarmFilter;
	SaNtfAttributeChangeNotificationFilterT attChFilter;
	SaNtfStateChangeNotificationFilterT stChFilter;
	SaNtfObjectCreateDeleteNotificationFilterT objCrDelFilter;
	SaNtfSecurityAlarmNotificationFilterT secAlarmFilter;
	
	SaNtfNotificationTypeFilterHandlesT notificationFilterHandles;
	memset(&notificationFilterHandles, 0, sizeof notificationFilterHandles);

	if (used_filters.all || used_filters.alarm) {
		errorCode = saNtfAlarmNotificationFilterAllocate(ntfHandle,
								 &myAlarmFilter,
								 notificationFilterAllocationParams->numEventTypes,
								 notificationFilterAllocationParams->numNotificationObjects,
								 notificationFilterAllocationParams->numNotifyingObjects,
								 notificationFilterAllocationParams->numNotificationClassIds,
								 notificationFilterAllocationParams->numProbableCauses,
								 notificationFilterAllocationParams->numPerceivedSeverities,
								 notificationFilterAllocationParams->numTrends);
	
		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saNtfAlarmNotificationFilterAllocate failed - %s\n", error_output(errorCode));
			return errorCode;
		}
		notificationFilterHandles.alarmFilterHandle = myAlarmFilter.notificationFilterHandle;
	}
	
	if (used_filters.all || used_filters.att_ch) {
		errorCode = saNtfAttributeChangeNotificationFilterAllocate(ntfHandle,
			&attChFilter,
			notificationFilterAllocationParams->numEventTypes,
			notificationFilterAllocationParams->numNotificationObjects,
			notificationFilterAllocationParams->numNotifyingObjects,
			notificationFilterAllocationParams->numNotificationClassIds,
			0);

		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saNtfAttributeChangeNotificationFilterAllocate failed - %s\n", error_output(errorCode));
			return errorCode;
		}
		notificationFilterHandles.attributeChangeFilterHandle = attChFilter.notificationFilterHandle;
	}
	
	if (used_filters.all || used_filters.obj_cr_del) {
		errorCode = saNtfObjectCreateDeleteNotificationFilterAllocate(ntfHandle,
			&objCrDelFilter,
			notificationFilterAllocationParams->numEventTypes,
			notificationFilterAllocationParams->numNotificationObjects,
			notificationFilterAllocationParams->numNotifyingObjects,
			notificationFilterAllocationParams->numNotificationClassIds,
			0);

		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saNtfObjectCreateDeleteNotificationFilterAllocate failed - %s\n", error_output(errorCode));
			return errorCode;
		}
		notificationFilterHandles.objectCreateDeleteFilterHandle = objCrDelFilter.notificationFilterHandle;
	}

	if (used_filters.all || used_filters.st_ch) {
		errorCode = saNtfStateChangeNotificationFilterAllocate(ntfHandle,
			&stChFilter,
			notificationFilterAllocationParams->numEventTypes,
			notificationFilterAllocationParams->numNotificationObjects,
			notificationFilterAllocationParams->numNotifyingObjects,
			notificationFilterAllocationParams->numNotificationClassIds,
			0,
			0);
		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saNtfStateChangeNotificationFilterAllocate failed - %s\n", error_output(errorCode));
			return errorCode;
		}
		notificationFilterHandles.stateChangeFilterHandle = stChFilter.notificationFilterHandle;
	}
	
	if (used_filters.all || used_filters.sec_al) {
		errorCode = saNtfSecurityAlarmNotificationFilterAllocate(ntfHandle,
			&secAlarmFilter,
			notificationFilterAllocationParams->numEventTypes,
			notificationFilterAllocationParams->numNotificationObjects,
			notificationFilterAllocationParams->numNotifyingObjects,
			notificationFilterAllocationParams->numNotificationClassIds,
			0,0,0,0,0);
		if (errorCode != SA_AIS_OK) {
			fprintf(stderr, "saNtfSecurityAlarmNotificationFilterAllocate failed - %s\n", error_output(errorCode));
			return errorCode;
		}
		notificationFilterHandles.securityAlarmFilterHandle = secAlarmFilter.notificationFilterHandle;
	}

	
	errorCode = saNtfNotificationSubscribe(&notificationFilterHandles, subscriptionId);
	if (SA_AIS_OK != errorCode) {
		fprintf(stderr, "saNtfNotificationSubscribe failed - %s\n", error_output(errorCode));
		return errorCode;
	}
	freeNtfFilter (&notificationFilterHandles.alarmFilterHandle);
	freeNtfFilter (&notificationFilterHandles.attributeChangeFilterHandle);
	freeNtfFilter (&notificationFilterHandles.objectCreateDeleteFilterHandle);
	freeNtfFilter (&notificationFilterHandles.stateChangeFilterHandle);
	freeNtfFilter (&notificationFilterHandles.securityAlarmFilterHandle);

	return errorCode;
}

int main(int argc, char *argv[])
{
	int c;
	SaAisErrorT error;
	int timeout = -1;	/* block indefintively in poll */
	saNotificationFilterAllocationParamsT notificationFilterAllocationParams = {0};
	SaNtfSubscriptionIdT subscriptionId = 1;
	struct option long_options[] = {
		{"alarm", no_argument, 0, 'a'},
		{"attributeChange", no_argument, 0, 'c'},
		{"objectCreateDelete", no_argument, 0, 'o'},
		{"stateChange", no_argument, 0, 's'},
		{"securityAlarm", no_argument, 0, 'y'},
		{"help", no_argument, 0, 'h'},
		{"timeout", required_argument, 0, 't'},
		{"verbose", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	progname = argv[0];

	/* Check options */
	while (1) {
		c = getopt_long(argc, argv, "acosyht:v", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
		case 'a':
			used_filters.all = 0;
			used_filters.alarm = 1;
			break;
		case 'o':
			used_filters.all = 0;
			used_filters.obj_cr_del = 1;
			break;
		case 'c':
			used_filters.all = 0;
			used_filters.att_ch = 1;
			break;
		case 's':
			used_filters.all = 0;
			used_filters.st_ch = 1;
			break;
		case 'y':
			used_filters.all = 0;
			used_filters.sec_al = 1;
			break;
		case 't':
			timeout = atoi(optarg) * 1000;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'h':
		case '?':
		default:
			usage();
			break;
		}
	}

	error = saNtfInitialize(&ntfHandle, &ntfCallbacks, &version);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "saNtfInitialize failed - %s\n", error_output(error));
		exit(EXIT_FAILURE);
	}

	error = saNtfSelectionObjectGet(ntfHandle, &selObj);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "saNtfSelectionObjectGet failed - %s\n", error_output(error));
		exit(EXIT_FAILURE);
	}

	error = subscribeForNotifications(&notificationFilterAllocationParams, subscriptionId);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "subscribeForNotifications failed - %s\n", error_output(error));
		exit(EXIT_FAILURE);
	}

	error = waitForNotifications(ntfHandle, selObj, timeout);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "subscribeForNotifications failed - %s\n", error_output(error));
		exit(EXIT_FAILURE);
	}

	error = saNtfNotificationUnsubscribe(subscriptionId);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "waitForNotifications failed - %s\n", error_output(error));
		exit(EXIT_FAILURE);
	}

	error = saNtfFinalize(ntfHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "saNtfFinalize failed - %s\n", error_output(error));
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

