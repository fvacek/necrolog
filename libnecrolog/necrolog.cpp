#include "necrolog.h"

#include <ctime>
#include <algorithm>
#include <sstream>

#ifdef __unix
#include <unistd.h>
#endif

NecroLog::Options &NecroLog::globalOptions()
{
	static Options global_options;
	return global_options;
}

NecroLog NecroLog::create(std::ostream &os, Level level, LogContext &&log_context)
{
	return NecroLog(os, level, std::move(log_context));
}

bool NecroLog::shouldLog(Level level, const LogContext &context)
{
	Options &opts = NecroLog::globalOptions();
	const std::map<std::string, Level> &tresholds = opts.tresholds;
	if(level <= opts.defaultLogLevel)
		return true; // default log level

	const char *ctx_topic = (context.topic && context.topic[0])? context.topic: context.file;
	for (size_t j = 0; ctx_topic[j]; ++j) {
		for(const auto &pair : tresholds) {
			const std::string &g_topic = pair.first;
			size_t i;
			//printf("%s vs %s\n", ctx_topic, g_topic.data());
			for (i = 0; i < g_topic.size() && ctx_topic[i+j]; ++i) {
				if(tolower(ctx_topic[i+j]) != g_topic[i])
					break;
			}
			if(i == g_topic.size())
				return (level <= pair.second);
		}
	}
	return false;
}

std::vector<std::string> NecroLog::setCLIOptions(int argc, char *argv[])
{
	using namespace std;
	std::vector<string> ret;
	Options& options = NecroLog::globalOptions();
	for(int i=1; i<argc; i++) {
		string s = argv[i];
		if(s == "-lfn" || s == "--log-long-file-names") {
			i++;
			options.logLongFileNames = true;
		}
		else if(s == "-d" || s == "-v" || s == "--verbose") {
			i++;
			string tresholds = (i < argc)? argv[i]: string();
			if(!tresholds.empty() && tresholds[0] == '-') {
				i--;
				tresholds = ":D";
			}
			{
				// split on ','
				size_t pos = 0;
				while(true) {
					size_t pos2 = tresholds.find_first_of(',', pos);
					string topic_level = (pos2 == string::npos)? tresholds.substr(pos): tresholds.substr(pos, pos2 - pos);
					if(!topic_level.empty()) {
						auto ix = topic_level.find(':');
						Level level = Level::Debug;
						std::string topic = topic_level;
						if(ix != std::string::npos) {
							std::string s = topic_level.substr(ix + 1, 1);
							char l = s.empty()? 'D': toupper(s[0]);
							topic = topic_level.substr(0, ix);
							switch(l) {
							case 'D': level = Level::Debug; break;
							case 'W': level = Level::Warning; break;
							case 'E': level = Level::Error; break;
							case 'I':
							default: level = Level::Info; break;
							}
						}
						std::transform(topic.begin(), topic.end(), topic.begin(), ::tolower);
						if(topic.empty())
							options.defaultLogLevel = level;
						else
							options.tresholds[topic] = level;
					}
					if(pos2 == string::npos)
						break;
					pos = pos2 + 1;
				}
			}
		}
		else {
			ret.push_back(s);
		}
	}
	ret.insert(ret.begin(), argv[0]);
	return ret;
}

std::string NecroLog::tresholdsLogInfo()
{
	std::string ret;
	for (auto& kv : NecroLog::globalOptions().tresholds) {
		if(!ret.empty())
			ret += ',';
		ret += kv.first + ':';
		switch(kv.second) {
		case Level::Debug: ret += 'D'; break;
		case Level::Info: ret += 'I'; break;
		case Level::Warning: ret += 'W'; break;
		case Level::Error: ret += 'E'; break;
		case Level::Fatal: ret += 'F'; break;
		case Level::Invalid: ret += 'N'; break;
		default: ret += '?'; break;
		}
	}
	return ret;
}
/*
std::string NecroLog::instantiationInfo()
{
	std::ostringstream ss;
	ss << "Instantiation info: ";
	ss << "globalOptions address: " << (void*)&(NecroLog::globalOptions().logLongFileNames);
	ss << ", cliHelp literal address: " << (void*)NecroLog::cliHelp();
	ss << ", cnt: " << ++NecroLog::globalOptions().cnt;
	return ss.str();
}
*/
const char * NecroLog::cliHelp()
{
	static const char * ret =
		"-lfn, --log-long-file-names\n"
		"\tLog long file names\n"
		"-d, -v, --verbose [<pattern>]:[D|I|W|E]\n"
		"\tSet files or topics log treshold\n"
		"\tset treshold for all files or topics containing pattern to treshold D|I|W|E\n"
		"\twhen pattern is not set, set treshold for any filename or topic\n"
		"\twhen treshold is not set, set treshold D (Debug) for all files or topics containing pattern\n"
		"\twhen nothing is not set, set treshold D (Debug) for all files or topics\n"
		"\tExamples:\n"
		"\t\t-d\t\tset treshold D (Debug) for all files or topics\n"
		"\t\t-d :W\t\tset treshold W (Warning) for all files or topics\n"
		"\t\t-d foo,bar\t\tset treshold D for all files or topics containing 'foo' or 'bar'\n"
		"\t\t-d bar:W\tset treshold W (Warning) for all files or topics containing 'bar'\n"
		;
	return ret;
}

NecroLog::Necro::Necro(std::ostream &os, NecroLog::Level level, NecroLog::LogContext &&log_context)
	: m_os(os)
	, m_level(level)
	, m_logContext(std::move(log_context))
{
#ifdef __unix
	m_isTTI = true;//(&m_os == &std::clog) && ::isatty(STDERR_FILENO);
#endif
}

NecroLog::Necro::~Necro()
{
	epilog();
	m_os << std::endl;
	m_os.flush();
}

void NecroLog::Necro::maybeSpace()
{
	if(m_firstRun) {
		m_firstRun = false;
		prolog();
	}
	else {
		if(m_isSpace) {
			m_os << ' ';
		}
	}
}

std::string NecroLog::Necro::moduleFromFileName(const char *file_name)
{
	if(NecroLog::globalOptions().logLongFileNames)
		return std::string(file_name);

	std::string ret(file_name);
	auto ix = ret.find_last_of('/');
#ifndef __unix
	if(ix == std::string::npos)
		ix = ret.find_last_of('\\');
#endif
	if(ix != std::string::npos)
		ret = ret.substr(ix + 1);
	return ret;
}

std::ostream &NecroLog::Necro::setTtyColor(NecroLog::Necro::TTYColor color, bool bright, bool bg_color)
{
	if(m_isTTI)
		m_os << "\033[" << (bright? '1': '0') << ';' << (bg_color? '4': '3') << char('0' + color) << 'm';
	return m_os;
}

void NecroLog::Necro::prolog()
{
	std::time_t t = std::time(nullptr);
	std::tm *tm = std::gmtime(&t); /// gmtime is not thread safe!!!
	char buffer[80] = {0};
	std::strftime(buffer, sizeof(buffer),"%Y-%m-%dT%H:%M:%S", tm);
	setTtyColor(TTYColor::Green, false) << std::string(buffer);
	setTtyColor(TTYColor::Yellow, false) << '[' << moduleFromFileName(m_logContext.file) << ':' << m_logContext.line << "]";
	if(m_logContext.topic && m_logContext.topic[0]) {
		setTtyColor(TTYColor::White, true) << '(' << m_logContext.topic << ")";
	}
	switch(m_level) {
	case NecroLog::Level::Fatal:
		setTtyColor(TTYColor::Red, true) << "|F|";
		break;
	case NecroLog::Level::Error:
		setTtyColor(TTYColor::Red, true) << "|E|";
		break;
	case NecroLog::Level::Warning:
		setTtyColor(TTYColor::Magenta, true) << "|W|";
		break;
	case NecroLog::Level::Info:
		setTtyColor(TTYColor::Cyan, true) << "|I|";
		break;
	case NecroLog::Level::Debug:
		setTtyColor(TTYColor::White, false) << "|D|";
		break;
	default:
		setTtyColor(TTYColor::Red, true) << "|?|";
		break;
	};
	m_os << " ";
}

void NecroLog::Necro::epilog()
{
	if(m_isTTI)
		m_os << "\33[0m";
}