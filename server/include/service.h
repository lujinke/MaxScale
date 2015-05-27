#ifndef _SERVICE_H
#define _SERVICE_H
/*
 * This file is distributed as part of the MariaDB Corporation MaxScale.  It is free
 * software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright MariaDB Corporation Ab 2013-2014
 */

#include <time.h>
#include <spinlock.h>
#include <dcb.h>
#include <server.h>
#include <filter.h>
#include <hashtable.h>
#include <resultset.h>
#include <maxconfig.h>

/**
 * @file service.h
 *
 * The service level definitions within the gateway
 *
 * @verbatim
 * Revision History
 *
 * Date		Who			Description
 * 14/06/13	Mark Riddoch		Initial implementation
 * 18/06/13	Mark Riddoch		Addition of statistics and function
 * 					prototypes
 * 23/06/13	Mark Riddoch		Added service user and users
 * 06/02/14	Massimiliano Pinto	Added service flag for root user access
 * 25/02/14	Massimiliano Pinto	Added service refresh limit feature
 * 07/05/14	Massimiliano Pinto	Added version_string field to service
 *					struct
 * 29/05/14	Mark Riddoch		Filter API mechanism
 * 26/06/14	Mark Riddoch		Added WeightBy support
 * 09/09/14	Massimiliano Pinto	Added service option for localhost authentication
 * 09/10/14	Massimiliano Pinto	Added service resources via hashtable 
 *
 * @endverbatim
 */
struct	server;
struct	router;
struct	router_object;
struct	users;

/**
 * The servprotocol structure is used to link a service to the protocols that
 * are used to support that service. It defines the name of the protocol module
 * that should be loaded to support the client connection and the port that the
 * protocol should use to listen for incoming client connections.
 */
typedef struct servprotocol {
	char		*protocol;	/**< Protocol module to load */
	unsigned short	port;		/**< Port to listen on */
	char		*address;	/**< Address to listen with */
	DCB		*listener;	/**< The DCB for the listener */
	struct	servprotocol
			*next;		/**< Next service protocol */
} SERV_PROTOCOL;

/**
 * The service statistics structure
 */
typedef struct {
	time_t		started;	/**< The time when the service was started */
	int		n_sessions;	/**< Number of sessions created on service since start */
	int		n_current;	/**< Current number of sessions */
} SERVICE_STATS;

/**
 * The service user structure holds the information that is needed
 for this service to allow the gateway to login to the backend
 database and extact information such as the user table or other
 database status or configuration data.
 */
typedef struct {
	char		*name;		/**< The user name to use to extract information */
	char		*authdata;	/**< The authentication data requied */
} SERVICE_USER;

/**
 * The service refresh rate holds the counter and last load time_t
 for this service to load users data from the backend database
 */
typedef struct {
	int nloads;
	time_t last;
} SERVICE_REFRESH_RATE;

typedef struct server_ref_t{
        struct server_ref_t *next;
        SERVER* server;
}SERVER_REF;

/**
 * Defines a service within the gateway.
 *
 * A service is a combination of a set of backend servers, a routing mechanism
 * and a set of client side protocol/port pairs used to listen for new connections
 * to the service.
 */
typedef struct service {
	char		*name;		/**< The service name */
	int		state;		/**< The service state */
	SERV_PROTOCOL	*ports;		/**< Linked list of ports and protocols
					 * that this service will listen on.
					 */
	char		*routerModule;	/**< Name of router module to use */
	char		**routerOptions;/**< Router specific option strings */
	struct router_object
			*router;	/**< The router we are using */
	void		*router_instance;
					/**< The router instance for this service */
	char            *version_string;/** version string for this service listeners */
        SERVER_REF      *servers;         /** server references */
	SERVICE_USER	credentials;	/**< The cedentials of the service user */	
	SPINLOCK	spin;		/**< The service spinlock */
	SERVICE_STATS	stats;		/**< The service statistics */
	struct users	*users;		/**< The user data for this service */
	int		enable_root;	/**< Allow root user  access */
	int		localhost_match_wildcard_host; /**< Match localhost against wildcard */
	HASHTABLE	*resources;	/**< hastable for service resources, i.e. database names */
	CONFIG_PARAMETER*
			svc_config_param;     /*<  list of config params and values */
	int             svc_config_version;   /*<  Version number of configuration */
	bool            svc_do_shutdown;	/*< tells the service to exit loops etc. */
        bool            users_from_all;         /*< Load users from one server or all of them */
        bool            strip_db_esc;      /*< Remove the '\' characters from database names
                                            * when querying them from the server. MySQL Workbench seems
                                            * to escape at least the underscore character. */
        bool optimize_wildcard;             /*< Convert wildcard grants to individual database grants */
	SPINLOCK
			users_table_spin;	/**< The spinlock for users data refresh */
	SERVICE_REFRESH_RATE
			rate_limit;		/**< The refresh rate limit for users table */
	FILTER_DEF	**filters;		/**< Ordered list of filters */
	int		n_filters;		/**< Number of filters */
        int             conn_timeout;           /*< Session timeout in seconds */
	char		*weightby;
	struct service	*next;			/**< The next service in the linked list */
} SERVICE;

typedef enum count_spec_t {COUNT_NONE=0, COUNT_ATLEAST, COUNT_EXACT, COUNT_ATMOST} count_spec_t;

#define	SERVICE_STATE_ALLOC	1	/**< The service has been allocated */
#define	SERVICE_STATE_STARTED	2	/**< The service has been started */
#define	SERVICE_STATE_FAILED	3	/**< The service failed to start */
#define	SERVICE_STATE_STOPPED	4	/**< The service has been stopped */

extern	SERVICE *service_alloc(const char *, const char *);
extern	int	service_free(SERVICE *);
extern	SERVICE *service_find(char *);
extern	int	service_isvalid(SERVICE *);
extern	int	serviceAddProtocol(SERVICE *, char *, char *, unsigned short);
extern	int	serviceHasProtocol(SERVICE *, char *, unsigned short);
extern	void	serviceAddBackend(SERVICE *, SERVER *);
extern	int	serviceHasBackend(SERVICE *, SERVER *);
extern  void    serviceClearBackends(SERVICE *service);
extern	void	serviceAddRouterOption(SERVICE *, char *);
extern	void	serviceClearRouterOptions(SERVICE *);
extern	int	serviceStart(SERVICE *);
extern	int	serviceStartAll();
extern	void	serviceStartProtocol(SERVICE *, char *, int);
extern	int	serviceStop(SERVICE *);
extern	int	serviceRestart(SERVICE *);
extern	int	serviceSetUser(SERVICE *, char *, char *);
extern	int	serviceGetUser(SERVICE *, char **, char **);
extern	void	serviceSetFilters(SERVICE *, char *);
extern  void    serviceUpdateFilters(SERVICE *service, CONFIG_CONTEXT *context);
extern  void    serviceUpdateRouter(SERVICE *service, CONFIG_CONTEXT *context);
extern  int     serviceUpdateNamedFilter(char* name, CONFIG_CONTEXT *context);
extern	int	serviceEnableRootUser(SERVICE *, int );
extern	int	serviceSetTimeout(SERVICE *, int );
extern	void	serviceWeightBy(SERVICE *, char *);
extern	char	*serviceGetWeightingParameter(SERVICE *);
extern	int	serviceEnableLocalhostMatchWildcardHost(SERVICE *, int);
int serviceStripDbEsc(SERVICE* service, int action);
int serviceAuthAllServers(SERVICE *service, int action);
int serviceOptimizeWildcard(SERVICE *service, int action);
extern	void	service_update(SERVICE *, char *, char *, char *);
extern	int	service_refresh_users(SERVICE *);
extern	void	printService(SERVICE *);
extern	void	printAllServices();
extern	void	dprintAllServices(DCB *);

bool service_set_param_value (
        SERVICE*            service,
        CONFIG_PARAMETER*   param,
        char*               valstr,
        count_spec_t        count_spec,
        config_param_type_t type);

extern	void	dprintService(DCB *, SERVICE *);
extern	void	dListServices(DCB *);
extern	void	dListListeners(DCB *);
char* service_get_name(SERVICE* svc);
void  service_shutdown();
extern	int	serviceSessionCountAll();
extern  RESULTSET	*serviceGetList();
extern  RESULTSET	*serviceGetListenerList();
#endif
