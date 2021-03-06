#include <Windows.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <AuthZ.h>
#include <sddl.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "Authz.lib")


// |     ..:: Data structures ::..     |
// -------------------------------------
/// Predefined default Windows event codes
enum EvType {
	// Security event types
	AUDIT_FAIL = 0x0000000,
	AUDIT_SUCCESS = 0x00000001,
	// --------------------
	EV_ERROR = 0x0001,
	EV_INFO = 0x0004,
	EV_WARNING = 0x0002,
	SUCCESS = 0x0000 // Deprecated
};

/// Required arguments for a ReportEvent call.
/// Binary data supported, however, uninplemented.
struct EvParams {
	int count = 1;				// Number of repeated event reports
	const char* eventLog = "Windows Error Reporting";	// Event source
	EvType type = EV_WARNING;	// Event type
	WORD category = 0x0;		// Event category/"task"
	DWORD ID = 1001;			// Event ID
	DWORD qualifier = 0;		// Event qualifier
	const char* UID = NULL;		// Windows user ID
	const char* content[50] = {NULL};	// Event description
	DWORD binSize = 0;			// Binary data size - TO BE IMPLEMENTED
	bool sec = false;			// Writing to security log
	int dtCount = 0;			// Count of content[] itens
	const char* server = NULL;	// Server to which report event
};
// -------------------------------------


// |     ..:: Function prototypes ::..     |
// -----------------------------------------
EvParams handleArgs(int argc, char* argv[]);

EvType getEvType(std::string arg);
EvType getEvType(const char* arg);

int errChkstoi(const char* str);
// -----------------------------------------

int main(int argc, char* argv[])
{
	EvParams eventArgs = handleArgs(argc, argv);
	HANDLE eventLog = NULL;
	PSID usid = NULL;
	// The event ID's first 16 bits make the qualifier, while the remaining 16 bits correspond to the acual ID.
	// eg. 0xqqqqiiii
	DWORD eventID = eventArgs.qualifier * pow(2, 16) + eventArgs.ID;
	eventLog = RegisterEventSourceA(eventArgs.server, eventArgs.eventLog);
	if (eventLog) {
		std::cout << "Error while registering event source.\nError code:" << GetLastError();
		std::cout << "For more information regaring error codes refer to Microsoft system error codes.";
		return GetLastError();
	}
	if (eventArgs.UID)
		ConvertStringSidToSidA(eventArgs.UID, &usid);

	for (int i = 0; i < eventArgs.count; i++)
		if (!eventArgs.sec)
			ReportEventA(eventLog, eventArgs.type, eventArgs.category, eventID, usid, eventArgs.dtCount, 0, eventArgs.content, NULL);
		else {
			int err = 0;
			AUTHZ_SECURITY_EVENT_PROVIDER_HANDLE hEventProvider = NULL;
			wchar_t wtext[1024];
			size_t outsz;
			if (strlen(eventArgs.eventLog) <= 1024)
				mbstowcs_s(&outsz, wtext, strlen(eventArgs.eventLog) + 1, eventArgs.eventLog, strlen(eventArgs.eventLog));
			PCWSTR ptext = wtext;
			err = AuthzRegisterSecurityEventSource(0, ptext, &hEventProvider);
			if (err) {
				std::wcout << "\n" << "Error while registering security event source.\nError code: " << GetLastError();
				std::cout << "For more information regaring error codes refer to Microsoft system error codes.";
				return GetLastError();
			}
			err = AuthzReportSecurityEvent(eventArgs.type, hEventProvider, eventID, usid, eventArgs.dtCount, eventArgs.content);
			if (err) {
				std::wcout << "\n" << "Error while reporting event source.\nError code: " << GetLastError();
				std::cout << "For more information regaring error codes refer to Microsoft system error codes.";
				return GetLastError();
			}
		}
	return 0;

	//(invisible) Hello World! For good measure:
    //std::cout << "Hello World!\n";

	DeregisterEventSource(eventLog);
}

// |     ..:: Function declarations ::..     |
// -------------------------------------------
/// Parses the command line arguments and initializes a EvParams "instance".
/// Defined default values used for missing arguments.
EvParams handleArgs(int argc, char* argv[]) {
	EvParams evparams;
	for (int i = 1; i < argc; i ++) {
		int st_i = i + 1;
		
		if (argv[i][0] == '/' || argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'c':
				if(i < argc - 1)
					evparams.count = errChkstoi(argv[i + 1]);
				else goto helpcase;
				break;
			case 's':
				if (i < argc - 1)
					evparams.eventLog = argv[i + 1];
				else goto helpcase;
				break;
			case 't':
				if (i < argc - 1)
					evparams.type = getEvType(argv[i + 1]);
				else goto helpcase;
				break;
			case 'C':
				if (i < argc - 1)
					evparams.category = errChkstoi(argv[i + 1]);
				else goto helpcase;
				break;
			case 'i':
				if (i < argc - 1)
					evparams.ID = errChkstoi(argv[i + 1]);
				else goto helpcase;
				break;
			case 'd':
				if (i < argc - 1)
					while (++i < argc && argv[i][0] != '/' && argv[i][0] != '-' && i - st_i < 50) {
						evparams.content[i - st_i] = argv[i];
						evparams.dtCount++;
					}
				else goto helpcase;
				i--;
				break;
			case 'S':
				evparams.sec = true;
				break;
			case 'u':
				if (i < argc - 1)
					evparams.UID = argv[i + 1];
				else goto helpcase;
				break;
			case 'q':
				if (i < argc - 1)
					evparams.qualifier = errChkstoi(argv[i + 1]);
				else goto helpcase;
				break;
			case 'v':
				if (i < argc - 1)
					evparams.server = argv[i + 1];
				else goto helpcase;
				break;
		helpcase:
			case 'h':
				std::cout << "Windows Event Injector\nUsage: " << argv[0] << " [-S] [-c count] [-s eventSource] [-t eventType] [-C eventCategory] [-i eventID] [-d eventDescription] [-u uid] [-q qualifier] [-v server]" << "\n"
						  << "   -c count: Number of times to repeat event.\n" << "   -s eventSource: Source, also called \"Provider Name\" in Windows Event Viewer." << "\n"
						  << "   -t eventType: Event \"level\".\n                 Available options - SUCCESS; AUDIT_FAIL; AUDIT_SUCCESS; ERROR; INFO; WARNING" << "\n"
						  << "   -C eventCategory: Also called \"Task\" and \"Task Category\" in Windows Event Viewer." << "\n"
						  << "   -i eventID: Event ID.\n" << "   -d eventDescription: Data to be passed to event viewer, should be composed of space separated string/values(limited to 50)." << "\n"
						  << "                        The number of parameters is event ID and event source dependent." << "\n"
						  << "                        Custom messages for evIDs/evSrc may be created using usual event creation process, creating/editing registry keys." << "\n"
						  << "                        Example usage: '-d arg1 arg2 3333 \"argument 4\"'" << "\n"
						  << "   -u uid: User ID to be used for event report. String UID format(S-N-N-...-N)" << "\n"
						  << "   -q qualifier: Event ID qualifier. Used in a few events to differentiate sources." << "\n"
						  << "   -v server: UNC server name to which report event." << "\n"
						  << "   -S: Writes to security log. Event source and ID required." << "\n"
						  << "   -h: This message.\n" << "All arguments are optional, the default behaviour is to fire a " << "\n"
						  << "\"Windows Error Reporting\" event once with no description, 1001 event ID and 0x0 category." << "\n"
						  << "Parameters may be defined by \"/\" or \"-\"." << "\n"
						  << "For more information regarding event IDs and expected parameters for each, refer to Windows .evt documentation." << "\n"
						  << "Hexadecimal values for event IDs/categories/types are not currently supported.\n";
				exit(0);
				break;
			default:
				std::cout << "Unknown argument " << argv[i] << "\nUse -h for help.\n";
				exit(1);
				break;
			}
		}
	}
	return evparams;
}

/// Parses an string to an EvType, which are simply predefined event type codes.
/// (case insensitive)
EvType getEvType(std::string arg) {
	std::transform(arg.begin(), arg.end(), arg.begin(), ::toupper);
	if (arg == "SUCCESS") return SUCCESS;
	else if (arg == "AUDIT_FAIL") return AUDIT_FAIL;
	else if (arg == "AUDIT_SUCCESS") return AUDIT_SUCCESS;
	else if (arg == "ERROR") return EV_ERROR;
	else if (arg == "INFO") return EV_INFO;
	else if (arg == "WARNING") return EV_WARNING;
	else std::cout << "Invalid event type " << arg << "\nFor help, use -h.\n";
		 exit(1);
}

// Unnecessary overload to avoid eventual compiler warnings.
EvType getEvType(const char* arg) {
	std::string str(arg);
	return getEvType(str);
}

/// ERRor CHecKed std::stoi.
/// Simply returns the parsed int if valid, otherwise outputs an error and exits.
int errChkstoi(const char* str) {
	try { return std::stoi(str); }
	catch (std::exception) { std::cout << "Invalid argument " << str << "\n"; exit(1); }
}
// -------------------------------------------
