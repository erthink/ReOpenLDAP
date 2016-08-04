/*
 * check_password.c for OpenLDAP
 *
 * See README
 */

#include <string.h>
#include <ctype.h>
#include <reldap.h>
#include <slap.h>

#ifdef HAVE_CRACKLIB
#	include <crack.h>
#endif

#ifdef CHECKPASSWD_DEBUG
#	include <syslog.h>
#endif

#ifndef CRACKLIB_DICTPATH
#	define CRACKLIB_DICTPATH "/usr/share/cracklib/pw_dict"
#endif

#ifndef CHECKPASSWD_CONFIGFILE
#	define CHECKPASSWD_CONFIGFILE "/etc/reopenldap/check_password.conf"
#endif

#define DEFAULT_QUALITY  3
#define DEFAULT_CRACKLIB 1
#define MEMORY_MARGIN    50
#define MEM_INIT_SZ      64
#define FILENAME_MAXLEN  512

#define PASSWORD_TOO_SHORT_SZ \
	"Password for dn=\"%s\" is too short (%d/6)"
#define PASSWORD_QUALITY_SZ \
	"Password for dn=\"%s\" does not pass required number of strength checks for the required character sets (%d of %d)"
#define BAD_PASSWORD_SZ \
	"Bad password for dn=\"%s\" because %s"
#define UNKNOWN_ERROR_SZ \
	"An unknown error occurred, please see your systems administrator"

typedef int (*validator) (char*);
static int read_config_file ();
static validator valid_word (char *);
static int set_quality (char *);
static int set_cracklib (char *);

int check_password (char *pPasswd, char **ppErrStr, Entry *pEntry);

struct config_entry {
	char* key;
	char* value;
	char* def_value;
} config_entries[] = {
	{ "minPoints", NULL, "3" },
	{ "useCracklib", NULL, "1" },
	{ "minUpper", NULL, "0" },
	{ "minLower", NULL, "0" },
	{ "minDigit", NULL, "0" },
	{ "minPunct", NULL, "0" },
	{ NULL, NULL, NULL }
};

int get_config_entry_int(char* entry) {
	struct config_entry* centry = config_entries;

	int i = 0;
	char* key = centry[i].key;
	while (key != NULL) {
		if ( strncmp(key, entry, strlen(key)) == 0 ) {
			if ( centry[i].value == NULL ) {
				return atoi(centry[i].def_value);
			}
			else {
				return atoi(centry[i].value);
			}
		}
		i++;
		key = centry[i].key;
	}

	return -1;
}

void dealloc_config_entries() {
	struct config_entry* centry = config_entries;

	int i = 0;
	while (centry[i].key != NULL) {
		if ( centry[i].value != NULL ) {
			ber_memfree(centry[i].value);
		}
		i++;
	}
}

char* chomp(char *s)
{
	char* t = ber_memalloc(strlen(s)+1);
	strncpy (t,s,strlen(s)+1);

	if ( t[strlen(t)-1] == '\n' ) {
		t[strlen(t)-1] = '\0';
	}

	return t;
}

static int set_quality (char *value)
{
#ifdef CHECKPASSWD_DEBUG
	Log(LDAP_DEBUG_CONFIG, LOG_NOTICE, "check_password: Setting quality to [%s]", value);
#endif

	/* No need to require more quality than we can check for. */
	if (!isdigit(*value) || (int) (value[0] - '0') > 4) return DEFAULT_QUALITY;
	return (int) (value[0] - '0');
}

static int set_cracklib (char *value)
{
#ifdef CHECKPASSWD_DEBUG
	Log(LDAP_DEBUG_CONFIG, LOG_NOTICE, "check_password: Setting cracklib usage to [%s]", value);
#endif

	return (int) (value[0] - '0');

}

static int set_digit (char *value)
{
#ifdef CHECKPASSWD_DEBUG
	Log(LDAP_DEBUG_CONFIG, LOG_NOTICE, "check_password: Setting parameter to [%s]", value);
#endif
	if (!isdigit(*value) || (int) (value[0] - '0') > 9) return 0;
	return (int) (value[0] - '0');
}

static validator valid_word (char *word)
{
	struct {
		char * parameter;
		validator dealer;
	} list[] = { { "minPoints", set_quality },
		     { "useCracklib", set_cracklib },
		     { "minUpper", set_digit },
		     { "minLower", set_digit },
		     { "minDigit", set_digit },
		     { "minPunct", set_digit },
		     { NULL, NULL } };
	int index = 0;

#ifdef CHECKPASSWD_DEBUG
	Log(LDAP_DEBUG_CONFIG, LOG_NOTICE, "check_password: Validating parameter [%s]", word);
#endif

	while (list[index].parameter != NULL) {
		if (strlen(word) == strlen(list[index].parameter) &&
		    strcmp(list[index].parameter, word) == 0) {
#ifdef CHECKPASSWD_DEBUG
			Log(LDAP_DEBUG_CONFIG, LOG_NOTICE, "check_password: Parameter accepted.");
#endif
			return list[index].dealer;
		}
		index++;
	}

#ifdef CHECKPASSWD_DEBUG
	Log(LDAP_DEBUG_ANY, LOG_NOTICE, "check_password: Parameter rejected.");
#endif

	return NULL;
}

static int read_config_file ()
{
	FILE * config;
	char * line;
	int returnValue =  -1;

	line = ber_memcalloc(260, sizeof(char));

	if ( line == NULL ) {
		return returnValue;
	}

	if ( (config = fopen(CHECKPASSWD_CONFIGFILE, "r")) == NULL) {
#ifdef CHECKPASSWD_DEBUG
		Log(LDAP_DEBUG_ANY, LOG_ERR, "check_password: Opening file %s failed", CHECKPASSWD_CONFIGFILE);
#endif

		ber_memfree(line);
		return returnValue;
	}

	returnValue = 0;

	while (fgets(line, 256, config) != NULL) {
		char *start = line;
		char *word, *value;
		validator dealer;

#ifdef CHECKPASSWD_DEBUG
		/* Debug traces to syslog. */
		Log(LDAP_DEBUG_TRACE, LOG_NOTICE, "check_password: Got line |%s|", line);
#endif

		while (isspace(*start) && isascii(*start)) start++;

		/* If we've got punctuation, just skip the line. */
		if ( ispunct(*start)) {
#ifdef CHECKPASSWD_DEBUG
			/* Debug traces to syslog. */
			Log(LDAP_DEBUG_TRACE, LOG_NOTICE, "check_password: Skipped line |%s|", line);
#endif
			continue;
		}

		if( isascii(*start)) {

			struct config_entry* centry = config_entries;
			int i = 0;
			char* keyWord = centry[i].key;
			if ((word = strtok(start, " \t")) && (value = strtok(NULL, " \t"))) {
				while ( keyWord != NULL ) {
					if ((strncmp(keyWord,word,strlen(keyWord)) == 0) && (dealer = valid_word(word)) ) {

#ifdef CHECKPASSWD_DEBUG
						Log(LDAP_DEBUG_TRACE, LOG_NOTICE, "check_password: Word = %s, value = %s", word, value);
#endif

						centry[i].value = chomp(value);
						break;
					}
					i++;
					keyWord = centry[i].key;
				}
			}
		}
	}
	fclose(config);
	ber_memfree(line);

	return returnValue;
}

static int realloc_error_message (char ** target, int curlen, int nextlen)
{
	if (curlen < nextlen + MEMORY_MARGIN) {
#ifdef CHECKPASSWD_DEBUG
		Log(LDAP_DEBUG_TRACE, LOG_WARNING, "check_password: Reallocating szErrStr from %d to %d",
		       curlen, nextlen + MEMORY_MARGIN);
#endif
		ber_memfree(*target);
		curlen = nextlen + MEMORY_MARGIN;
		*target = (char *) ber_memalloc(curlen);
	}

	return curlen;
}

int
check_password (char *pPasswd, char **ppErrStr, Entry *pEntry)
{

	char *szErrStr = (char *) ber_memalloc(MEM_INIT_SZ);
	int  mem_len = MEM_INIT_SZ;

	int nLen;
	int nLower = 0;
	int nUpper = 0;
	int nDigit = 0;
	int nPunct = 0;
	int minLower = 0;
	int minUpper = 0;
	int minDigit = 0;
	int minPunct = 0;
	int nQuality = 0;
	int i;

	/* Set a sensible default to keep original behaviour. */
	int minQuality = DEFAULT_QUALITY;
	int useCracklib = DEFAULT_CRACKLIB;

	/** bail out early as cracklib will reject passwords shorter
	 * than 6 characters
	 */

	nLen = strlen (pPasswd);
	if ( nLen < 6) {
		mem_len = realloc_error_message(&szErrStr, mem_len,
						strlen(PASSWORD_TOO_SHORT_SZ) +
						strlen(pEntry->e_name.bv_val) + 1);
		sprintf (szErrStr, PASSWORD_TOO_SHORT_SZ, pEntry->e_name.bv_val, nLen);
		goto fail;
	}

	if (read_config_file() == -1) {
		Log(LDAP_DEBUG_ANY, LOG_ERR, "Warning: Could not read values from config file %s. Using defaults.", CHECKPASSWD_CONFIGFILE);
	}

	minQuality = get_config_entry_int("minPoints");
	useCracklib = get_config_entry_int("useCracklib");
	minUpper = get_config_entry_int("minUpper");
	minLower = get_config_entry_int("minLower");
	minDigit = get_config_entry_int("minDigit");
	minPunct = get_config_entry_int("minPunct");

	/** The password must have at least minQuality strength points with one
	 * point for the first occurrance of a lower, upper, digit and
	 * punctuation character
	 */

	for ( i = 0; i < nLen; i++ ) {

		if ( islower (pPasswd[i]) ) {
			minLower--;
			if ( !nLower && (minLower < 1)) {
				nLower = 1; nQuality++;
#ifdef CHECKPASSWD_DEBUG
				Log(LDAP_DEBUG_TRACE, LOG_NOTICE, "check_password: Found lower character - quality raise %d", nQuality);
#endif
			}
			continue;
		}

		if ( isupper (pPasswd[i]) ) {
			minUpper--;
			if ( !nUpper && (minUpper < 1)) {
				nUpper = 1; nQuality++;
#ifdef CHECKPASSWD_DEBUG
				Log(LDAP_DEBUG_TRACE, LOG_NOTICE, "check_password: Found upper character - quality raise %d", nQuality);
#endif
			}
			continue;
		}

		if ( isdigit (pPasswd[i]) ) {
			minDigit--;
			if ( !nDigit && (minDigit < 1)) {
				nDigit = 1; nQuality++;
#ifdef CHECKPASSWD_DEBUG
				Log(LDAP_DEBUG_TRACE, LOG_NOTICE, "check_password: Found digit character - quality raise %d", nQuality);
#endif
			}
			continue;
		}

		if ( ispunct (pPasswd[i]) ) {
			minPunct--;
			if ( !nPunct && (minPunct < 1)) {
				nPunct = 1; nQuality++;
#ifdef CHECKPASSWD_DEBUG
				Log(LDAP_DEBUG_TRACE, LOG_NOTICE, "check_password: Found punctuation character - quality raise %d", nQuality);
#endif
			}
			continue;
		}
	}

	/*
	 * If you have a required field, then it should be required in the strength
	 * checks.
	 */

	if (
		(minLower > 0 ) ||
		(minUpper > 0 ) ||
		(minDigit > 0 ) ||
		(minPunct > 0 ) ||
		(nQuality < minQuality)
		) {
		mem_len = realloc_error_message(&szErrStr, mem_len,
						strlen(PASSWORD_QUALITY_SZ) +
						strlen(pEntry->e_name.bv_val) + 2);
		sprintf (szErrStr, PASSWORD_QUALITY_SZ, pEntry->e_name.bv_val,
			 nQuality, minQuality);
		goto fail;
	}

#ifdef HAVE_CRACKLIB

	/** Check password with cracklib */

	if ( useCracklib > 0 ) {
		int   j = 0;
		FILE* fp;
		char  filename[FILENAME_MAXLEN];
		char  const* ext[] = { "hwm", "pwd", "pwi" };
		int   nErr = 0;

		/**
		 * Silently fail when cracklib wordlist is not found
		 */

		for ( j = 0; j < 3; j++ ) {

			snprintf (filename, FILENAME_MAXLEN - 1, "%s.%s", \
				  CRACKLIB_DICTPATH, ext[j]);

			if (( fp = fopen ( filename, "r")) == NULL ) {

				nErr = 1;
				break;

			} else {
				fclose (fp);
			}
		}

		char *r;
		if ( nErr  == 0) {

			r = (char *) FascistCheck (pPasswd, CRACKLIB_DICTPATH);
			if ( r != NULL ) {
				mem_len = realloc_error_message(&szErrStr, mem_len,
								strlen(BAD_PASSWORD_SZ) +
								strlen(pEntry->e_name.bv_val) +
								strlen(r));
				sprintf (szErrStr, BAD_PASSWORD_SZ, pEntry->e_name.bv_val, r);
				goto fail;
			}
		}
	}

	else {
#ifdef CHECKPASSWD_DEBUG
		Log(LDAP_DEBUG_NONE, LOG_NOTICE, "check_password: Cracklib verification disabled by configuration");
#endif
	}

#endif
	dealloc_config_entries();
	*ppErrStr = strdup ("");
	ber_memfree(szErrStr);
	return (LDAP_SUCCESS);

fail:
	dealloc_config_entries();
	*ppErrStr = strdup (szErrStr);
	ber_memfree(szErrStr);
	return (EXIT_FAILURE);
}
