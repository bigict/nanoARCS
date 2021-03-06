#include "map.h"
#include "mole.h"
#include "runner.h"
#include "constant.h"

#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>

static log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("nanoARCS.main"));

int main(int argc, char* argv[]) {
    if(argc < 2) {
        return RunnerManager::instance()->help(argc, argv);
    }
    RunnerPtr runner = RunnerManager::instance()->get(argv[1]);
    if(!runner) {
        return RunnerManager::instance()->help(argc, argv);
    }
    Properties options;
    {
        //command line options.
        Properties cmd;
        const std::string& optstring = runner->options();
        int opt = -1;
        while((opt = getopt(argc - 1, argv + 1, optstring.c_str())) != -1) {
            std::string key = runner->transform((char)opt);
            if(optarg == NULL) {
                cmd.put(key, NULL);
            } else {
                std::string val = optarg;
                if(cmd.find(key) != cmd.not_found()) {
                    val = boost::str(boost::format("%s:%s") % cmd.get< std::string > (key) % optarg);
                }
                cmd.put(key, val);
            }
        }
        //log4cxx config
        const std::string log_config = cmd.get< std::string >("c", kLogConfig);
        if(boost::filesystem::exists(log_config)) {
            log4cxx::PropertyConfigurator::configure(log_config);
        } else {
            log4cxx::BasicConfigurator::configure();
        }
        //load ini options
        if(cmd.find("i") != cmd.not_found()) {
            const std::string config_file = cmd.get< std::string >("i");
            try {
                boost::property_tree::read_ini(config_file, options);
            } catch (const boost::property_tree::ini_parser_error& e) {
                LOG4CXX_ERROR(logger, boost::format("load %s failed(%s).") % config_file % e.what());
                return 1;
            }
        }
        //merge options
        for(auto it = cmd.begin(); it != cmd.end(); ++it) {
            options.put(it->first, it->second.data());
        }
    }
    Arguments arguments;
    std::copy(argv + optind + 1, argv + argc, std::back_inserter(arguments));
    return runner->run(options, arguments);
}
