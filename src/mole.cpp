#include "mole.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp> 
#include <boost/foreach.hpp>
#include <boost/format.hpp>

#include <log4cxx/logger.h>

static log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("nanoARCS.map"));

Mole Mole::reverseMole() {
    Mole reMole;
    reMole._id = "(-)" + _id;
    reMole._distance.assign(_distance.begin(), _distance.end());
    reverse(reMole._distance.begin(), reMole._distance.end());
    return reMole;
}

bool Mole::getDistance() {
    _distance.clear();
    for (int i = 0; i < _position.size() - 2; ++ i) {
        _distance.push_back(_position[i + 1] - _position[i]);
    }
    return true;
}

bool MoleReader::read(Mole& mole) {
    if(!_stream){
        return false;
    }
    enum {
        moleId,
        molePosition,
        moleQX11,
        moleQX12,
    };
    reset(mole);
    int state = moleId;
    std::string buf;
    std::vector<std::string> data;
    while (std::getline(_stream, buf)) {
        boost::algorithm::trim(buf);
        LOG4CXX_TRACE(logger, boost::format("line: %s") % buf);

        if (buf[0] == '#' || buf.empty()) continue;
        if (state == moleId) {
            boost::algorithm::split(data, buf, boost::algorithm::is_any_of(" \t"), boost::algorithm::token_compress_on);
            if (boost::lexical_cast<int>(data[0]) == 0) {
                mole._id = data[1];
                state = molePosition;
            } else {
                LOG4CXX_WARN(logger, boost::format("Load molecule file failed(invalid line for mole id: %s)") % mole._id);
                return false;
            }
        } else if (state == molePosition) {
            data = boost::algorithm::split(data, buf, boost::algorithm::is_any_of(" \t"), boost::algorithm::token_compress_on);
            if (boost::lexical_cast<int>(data[0]) == 1) {
                for (int i = 1; i < data.size(); ++ i) {
                    mole._position.push_back(static_cast< long > (boost::lexical_cast< double >(data[i])));
                }
                if (mole._position.size() > 1) {
                    mole.getDistance();
                }
                state = moleQX11;
            } else {
                LOG4CXX_WARN(logger, boost::format("Load molecule file failed(invalid line for mole position: %s)") % buf);
                return false;
            }
        } else if (state == moleQX11) {
            data = boost::algorithm::split(data, buf, boost::algorithm::is_any_of(" \t"), boost::algorithm::token_compress_on);
            if (data[0] == "QX11") {
                std::vector<double> qx11;
                for (int i = 1; i < data.size(); ++ i) {
                    qx11.push_back(static_cast< long > (boost::lexical_cast< double >(data[i])));
                }
                int l = 0;
                //theta is a threshold for SNR
                double theta = 3;
                //filter sites by QX11 SNR
                if(qx11.size() == mole._position.size()) {
                    for(int i = 0; i < mole._position.size(); ++ i) {
                        if(qx11[i] >= theta) {
                            mole._position[l ++] = mole._position[i];
                        }
                    }
                    mole._position.resize(l);
                }
                if (mole._position.size() > 1) {
                    mole.getDistance();
                }
                state = moleQX12;
            } else {
                LOG4CXX_WARN(logger, boost::format("bnx=>invalid line for mole QX11: %s") % buf);
                return false;
            }
            state = moleQX12;
        } else if (state == moleQX12) {
            state = moleId;
            return true;
        }
    }
    return false;
}
bool AlignmentReader::read(Alignment& al) {
    if(!_stream){
        return false;
    }
    std::string buf;
    std::vector<std::string> data;
    if(std::getline(_stream, buf)) {
        boost::algorithm::trim(buf);
        LOG4CXX_TRACE(logger, boost::format("line: %s") % buf);

        boost::algorithm::split(data, buf, boost::algorithm::is_any_of(" \t"), boost::algorithm::token_compress_on);
        al.mole1Id = data[0];
        al.mole2Id = data[1];
        al.score = boost::lexical_cast< double >(data[2]);

        al.mole1Start = boost::lexical_cast< int >(data[3]);
        al.mole2Start = boost::lexical_cast< int >(data[4]);
        al.mole1End = boost::lexical_cast< int >(data[5]);
        al.mole2End = boost::lexical_cast< int >(data[6]);

        std::getline(_stream, buf);
        boost::algorithm::trim(buf);
        LOG4CXX_TRACE(logger, boost::format("line: %s") % buf);
        boost::algorithm::split(data, buf, boost::algorithm::is_any_of(" \t"), boost::algorithm::token_compress_on);
        AlignedMole a1, a2;
        for(auto fragstr : data) {
            Fragment frag;
            std::vector<std::string> temp;
            boost::algorithm::split(temp, fragstr, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on);
            
            for(std::string intervalstr : temp) {
                frag.push_back(boost::lexical_cast< int >(intervalstr));
            }
            a1.push_back(frag);
        }
        std::getline(_stream, buf);
        boost::algorithm::trim(buf);
        LOG4CXX_TRACE(logger, boost::format("line: %s") % buf);
        boost::algorithm::split(data, buf, boost::algorithm::is_any_of(" \t"), boost::algorithm::token_compress_on);
        for(auto fragstr : data) {
            Fragment frag;
            std::vector<std::string> temp;
            boost::algorithm::split(temp, fragstr, boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on);
            for(std::string intervalstr : temp) {
                frag.push_back(boost::lexical_cast< int >(intervalstr));
            }
            a2.push_back(frag);
        }
        al.alignedMole1 = a1;
        al.alignedMole2 = a2;
        return true;
    }
    return false;
}
