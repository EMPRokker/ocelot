#include "ocelot.h"
#include "config.h"
#include "db.h"
#include "worker.h"
#include "events.h"
#include "schedule.h"
#include "logger.h"
#include "site_comm.h"

static mysql *db_ptr;
static connection_mother *mother;
static worker *work;
static logger *log_ptr;
static site_comm *sc_ptr;

static void sig_handler(int sig)
{
	std::cout << "Caught SIGINT/SIGTERM" << std::endl;
	if (work->signal(sig)) {
		exit(0);
	}
}

int main() {
	config conf;

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	log_ptr = new logger("debug.log");

	mysql db(conf.mysql_db, conf.mysql_host, conf.mysql_username, conf.mysql_password);
	db_ptr = &db;

	site_comm sc(conf);
	sc_ptr = &sc;
	
	std::vector<std::string> blacklist;
	db.load_blacklist(blacklist);
	std::cout << "Loaded " << blacklist.size() << " clients into the blacklist" << std::endl;
	if(blacklist.size() == 0) {
		std::cout << "Assuming no blacklist desired, disabling" << std::endl;
	}
	
	std::unordered_map<std::string, user> users_list;
	db.load_users(users_list);
	std::cout << "Loaded " << users_list.size() << " users" << std::endl;
	
	std::unordered_map<std::string, torrent> torrents_list;
	db.load_torrents(torrents_list);
	std::cout << "Loaded " << torrents_list.size() << " torrents" << std::endl;
        
	db.load_tokens(torrents_list);

        // Lanz: new site options struct, handles site wide freeleech for now.
        site_options_t site_options;
        db.load_site_options(site_options);
        
	// Create worker object, which handles announces and scrapes and all that jazz
	work = new worker(site_options, torrents_list, users_list, blacklist, &conf, &db, sc);
	
	// Create connection mother, which binds to its socket and handles the event stuff
	mother = new connection_mother(work, &conf, &db);

	return 0;
}
