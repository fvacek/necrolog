#include "necrolog.h"

#include <array>
#include <ctime>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <utility>

#ifdef __unix
#include <unistd.h>
#endif

namespace {
NecroLog::MessageHandler messageHandler = NecroLog::defaultMessageHandler; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
}

NecroLog::Options &NecroLog::globalOptions()
{
	static Options global_options;
	return global_options;
}

NecroLog NecroLog::create(Level level, const LogContext& log_context)
{
	return NecroLog(level, log_context);
}

bool NecroLog::shouldLog(Level level, const LogContext &context)
{
	//std::clog << levelToString(level) << " -> " << context.file() << ':' << context.line() << " topic: " << context.topic() << std::endl;
	Options &opts = NecroLog::globalOptions();

	const auto topic_set = context.isTopicSet();
	//if(topic_set)
	//	std::clog << &opts << " level: " << levelToString(level) << " topic: " << context.topic() << std::endl;
	//if(!topic_set && opts.fileThresholds.empty())
	//	return level <= Level::Info; // when thresholds are not set, log non-topic INFO messages

	const char *searched_str = nullptr;
	if(topic_set) {
		searched_str = context.topic();
	}
	else {
		searched_str = context.file();
		if(!opts.logLongFileNames) {
			int ix = moduleNameStart(searched_str);
			if(ix >= 0)
				searched_str += ix;
		}
	}

	//if(topic_set)
	//	std::clog << "\t searched str: " << searched_str << std::endl;

#define STARTS_WITH_ONLY

	const auto &thresholds = opts.topicThresholds;
	bool threshold_hit = false;
	for(const auto &kv : thresholds) {
		const std::string &needle = kv.first;
		//std::clog << "\tneedle: '" << needle << "' level: " << levelToString(kv.second) << " haystack: '" << searched_str << "'" << std::endl;
#ifdef STARTS_WITH_ONLY
		size_t i;
		for (i = 0; i < needle.size() && searched_str[i]; ++i) {
			if(opts.caseInsensitiveTopicMatch) {
				if(tolower(needle[i]) != tolower(searched_str[i]))
					break;
			}
			else {
				if(needle[i] != searched_str[i])
					break;
			}
		}
		if(i == needle.size()) {
			//std::clog << "needle: '" << needle << "' searched str: '" << searched_str << "' i: " << i << std::endl;
			threshold_hit = true;
			if(level <= kv.second)
				return true;
		}
#else
		for (size_t j = 0; searched_str[j]; ++j) {
			size_t i;
			for (i = 0; i < needle.size()  && searched_str[i+j]; ++i) {
				if(tolower(searched_str[i+j]) != needle[i])
					break;
			}
			if(i == needle.size()) {
				return true;
			}
		}
#endif
	}
	// not found in thresholds
	//std::clog << "threshold_hit: " << threshold_hit << context.file() << context.line() << std::endl;
	if(!threshold_hit) {
		if(topic_set) {
			return level <= Level::Warning;
		}
		return level <= Level::Info; // log non-topic INFO messages
	}
	return false;
}

NecroLog::MessageHandler NecroLog::setMessageHandler(NecroLog::MessageHandler h)
{
	return std::exchange(messageHandler, h);
}

namespace {
void parse_thresholds_string(const std::string &thresholds, std::map<std::string, NecroLog::Level> &threshold_map)
{
	using namespace std;
	threshold_map.clear();
	// split on ','
	size_t pos = 0;
	while(true) {
		size_t pos2 = thresholds.find_first_of(',', pos);
		string topic_level = (pos2 == string::npos)? thresholds.substr(pos): thresholds.substr(pos, pos2 - pos);
		if(!topic_level.empty()) {
			auto ix = topic_level.find(':');
			NecroLog::Level level = NecroLog::Level::Debug;
			std::string topic = topic_level;
			if(ix != std::string::npos) {
				std::string s = topic_level.substr(ix + 1, 1);
				char l = s.empty()? 'D': static_cast<char>(toupper(s[0]));
				topic = topic_level.substr(0, ix);
				switch(l) {
				case 'D': level = NecroLog::Level::Debug; break;
				case 'M': level = NecroLog::Level::Message; break;
				case 'W': level = NecroLog::Level::Warning; break;
				case 'E': level = NecroLog::Level::Error; break;
				case 'I':
				default: level = NecroLog::Level::Info; break;
				}
			}
			//std::transform(topic.begin(), topic.end(), topic.begin(), ::tolower);
			threshold_map[topic] = level;
		}
		if(pos2 == string::npos)
			break;
		pos = pos2 + 1;
	}
}
}

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
std::vector<std::string> NecroLog::setCLIOptions(int argc, char *argv[])
{
	std::vector<std::string> params;
	for (int i = 0; i < argc; ++i)
		params.emplace_back(argv[i]);
	return setCLIOptions(params);
}

std::vector<std::string> NecroLog::setCLIOptions(const std::vector<std::string> &params)
{
	using namespace std;
	std::vector<string> ret;
	Options& options = NecroLog::globalOptions();
	options = {};
	for(size_t i = 1; i < params.size(); i++) {
		const string s = params[i];
		if(s == "--lfn" || s == "--log-long-file-names") {
			i++;
			options.logLongFileNames = true;
		}
		else if(s == "-v" || s == "--vi" || s == "--verbose" || s == "--verbose-insensitive") {
			i++;
			string thresholds = (i < params.size())? params[i]: string();
			if(thresholds.empty() || (!thresholds.empty() && thresholds[0] == '-')) {
				i--;
				thresholds = ":D";
			}
			parse_thresholds_string(thresholds, options.topicThresholds);
			options.caseInsensitiveTopicMatch = s == "--vi" || s == "--verbose-insensitive";
		}
		else {
			ret.push_back(s);
		}
	}
	if(!params.empty())
		ret.insert(ret.begin(), params[0]);
	return ret;
}

void NecroLog::setTopicsLogThresholds(const std::string &thresholds)
{
	std::map<std::string, NecroLog::Level> &threshold_map = NecroLog::globalOptions().topicThresholds;
	threshold_map.clear();
	parse_thresholds_string(thresholds, threshold_map);
}

const char* NecroLog::levelToString(NecroLog::Level level)
{
	switch(level) {
	case NecroLog::Level::Debug: return "Debug";
	case NecroLog::Level::Info: return "Info";
	case NecroLog::Level::Message: return "Message";
	case NecroLog::Level::Warning: return "Warning";
	case NecroLog::Level::Error: return "Error";
	case NecroLog::Level::Fatal: return "Fatal";
	case NecroLog::Level::Invalid: return ""; // should not be used, even as string
	}
	return "";
}

NecroLog::Level NecroLog::stringToLevel(const char *str)
{
	if(str && *str != 0) {
		switch(str[0]) {
			case 'd':
			case 'D': return NecroLog::Level::Debug;
			case 'm':
			case 'M': return NecroLog::Level::Message;
			case 'i':
			case 'I': return NecroLog::Level::Info;
			case 'w':
			case 'W': return NecroLog::Level::Warning;
			case 'e':
			case 'E': return NecroLog::Level::Error;
			case 'f':
			case 'F': return NecroLog::Level::Fatal;
			default: return NecroLog::Level::Invalid;
		}
	}
	return NecroLog::Level::Invalid;
}

namespace {
std::string levels_to_string(const std::map<std::string, NecroLog::Level> &thresholds)
{
	std::string ret;
	for (auto& kv : thresholds) {
		if(!ret.empty())
			ret += ',';
		ret += kv.first + ':';
		const char *level_str = NecroLog::levelToString(kv.second);
		ret += level_str[0];
	}
	return ret;
}
}

std::string NecroLog::thresholdsLogInfo()
{
	std::string ret;
	Options &opts = NecroLog::globalOptions();
	if(!opts.topicThresholds.empty()) {
		ret += " -v ";
		ret += topicsLogThresholds();
	}
	return ret;
}

std::string NecroLog::topicsLogThresholds()
{
	Options &opts = NecroLog::globalOptions();
	return levels_to_string(opts.topicThresholds);
}
/*
std::string NecroLog::fileLogThresholds()
{
	std::string ret;
	Options &opts = NecroLog::globalOptions();
	return levels_to_string(opts.fileThresholds);
}

void NecroLog::setFileLogthresholds(const std::string &thresholds)
{
	std::map<std::string, NecroLog::Level> &threshold_map = NecroLog::globalOptions().fileThresholds;
	threshold_map.clear();
	parse_thresholds_string(thresholds, threshold_map);
}
*/
void NecroLog::registerTopic(const std::string &topic, const std::string &info)
{
	Options &opts = NecroLog::globalOptions();
	opts.registeredTopics[topic] = info;
}

std::string NecroLog::registeredTopicsInfo()
{
	std::string ret;
	Options &opts = NecroLog::globalOptions();
	if(!opts.registeredTopics.empty()) {
		ret += "Registered topics:\n";
		std::string topics;
		for(const auto &kv : opts.registeredTopics) {
			if(!topics.empty())
				topics += '\n';
			topics += '\t' + kv.first + "\t- " + kv.second;
		}
		ret += topics;
	}
	return ret;
}

NecroLog::ColorizedOutputMode NecroLog::colorizedOutputMode()
{
	return NecroLog::globalOptions().colorizedOutput;
}

void NecroLog::setColorizedOutputMode(ColorizedOutputMode mode)
{
	NecroLog::globalOptions().colorizedOutput = mode;
}

const char * NecroLog::cliHelp()
{
	static const char * ret =
		"--lfn, --log-long-file-names\n"
		"\tLog long file names\n"
		"-v, --verbose, --vi, --verbode-insensitive [<pattern>]:[D|I|W|E] set topic or module-name log threshold\n"
		"\tSet log threshold for all files or topics starting with pattern to threshold D|I|W|E\n"
		"Examples:\n"
		"\t\t\tif -v is not specified, set threshold I for all files and W for all topics\n"
		"\t-v\t\tset threshold D (Debug) for all files and topics\n"
		"\t-v :W\t\tset threshold W (Warning) for all files or topics\n"
		"\t-v foo,bar\t\tset threshold D for all files or topics starting with 'foo' or 'bar' and I for rest of files and W for rest of topics\n"
		"\t-v foo:W\t\tset threshold W for all files or topics starting with 'foo' or 'bar' and I for rest of files and W for rest of topics\n"
		;
	return ret;
}

NecroLog::NecroLogSharedData::NecroLogSharedData(NecroLog::Level level, const NecroLog::LogContext& log_context)
	: m_level(level)
	, m_logContext(log_context)
{
}

NecroLog::NecroLogSharedData::~NecroLogSharedData()
{
	if(messageHandler)
		messageHandler(m_level, m_logContext, m_os.str());
}

int NecroLog::moduleNameStart(const char *file_name)
{
	char sep = std::filesystem::path::preferred_separator;
	int ret = -1;
	if(file_name) {
		for (int ix = 0; file_name[ix]; ++ix) {
			if(file_name[ix] == sep)
				ret = ix+1;
		}
	}
	return ret;
}

std::string NecroLog::moduleFromFileName(const char *file_name)
{
	if(NecroLog::globalOptions().logLongFileNames)
		return std::string(file_name);

	int ix = moduleNameStart(file_name);
	if(ix < 0)
		return std::string(file_name);
	return std::string(file_name + ix);
}

namespace {

enum TTYColor {Black=0, Red, Green, Brown, Blue, Magenta, Cyan, LightGray, Gray, LightRed, LightGreen, Yellow, LightBlue, LightMagenta, LightCyan, White};

/// ESC[T;FG;BGm
/// T:
///   0 - normal
///   1 - bright
///   2 - dark
///   3 - italic
///   4 - underline
///   5 - blink
///   6 - ???
///   7 - inverse
///   8 - foreground same as background
/// FG:
///   30 - black
///   31 - red
///   32 - green
///   33 - yellow
///   34 - blue
///   35 - magenta
///   36 - cyan
///   37 - white
/// BG:
///   40 - black
///   41 - red
///   42 - green
///   43 - yellow
///   44 - blue
///   45 - magenta
///   46 - cyan
///   47 - white
std::ostream & set_TTY_color(bool is_tty, std::ostream &os, TTYColor color, bool bg_color = false)
{
	if(is_tty)
		os << "\033[" << ((color / 8)? '1': '0') << ';' << (bg_color? '4': '3') << char('0' + (color % 8)) << 'm';
	return os;
}

}

void NecroLog::defaultMessageHandler(NecroLog::Level level, const NecroLog::LogContext &context, const std::string &msg)
{
	std::ostream &os = std::clog;
	bool is_colorized = true;
	switch(NecroLog::colorizedOutputMode()) {
	case NecroLog::ColorizedOutputMode::IfTty: {
	#ifdef __unix
		static const bool is_tty = ::isatty(STDERR_FILENO);
	#else
		const bool is_tty = false;
	#endif
		is_colorized = is_tty;
		break;
	}
	case NecroLog::ColorizedOutputMode::Yes:
		is_colorized = true;
		break;
	case NecroLog::ColorizedOutputMode::No:
		is_colorized = false;
		break;

	}
	writeWithDefaultFormat(os, is_colorized, level, context, msg);
}

void NecroLog::writeWithDefaultFormat(std::ostream &os, bool is_colorized, NecroLog::Level level, const NecroLog::LogContext &context, const std::string &msg)
{
	std::time_t t = std::time(nullptr);
	std::tm *tm = std::gmtime(&t); /// gmtime is not thread safe!!!
	std::array<char, 80> buffer = {0};
	std::strftime(buffer.data(), buffer.size() * sizeof(*buffer.data()),"%Y-%m-%dT%H:%M:%S", tm);
	set_TTY_color(is_colorized, os, TTYColor::Green) << std::string(buffer.data());
	set_TTY_color(is_colorized, os, TTYColor::Brown) << '[' << moduleFromFileName(context.file()) << ':' << context.line() << "]";
	if(context.isTopicSet()) {
		set_TTY_color(is_colorized, os, TTYColor::White) << '(' << context.topic() << ")";
	}
	switch(level) {
	case NecroLog::Level::Fatal:
		set_TTY_color(is_colorized, os, context.isColorSet()? static_cast<TTYColor>(context.color()): TTYColor::LightRed) << "|F|";
		break;
	case NecroLog::Level::Error:
		set_TTY_color(is_colorized, os, context.isColorSet()? static_cast<TTYColor>(context.color()): TTYColor::LightRed) << "|E|";
		break;
	case NecroLog::Level::Warning:
		set_TTY_color(is_colorized, os, context.isColorSet()? static_cast<TTYColor>(context.color()): TTYColor::LightMagenta) << "|W|";
		break;
	case NecroLog::Level::Info:
		set_TTY_color(is_colorized, os, context.isColorSet()? static_cast<TTYColor>(context.color()): TTYColor::LightCyan) << "|I|";
		break;
	case NecroLog::Level::Message:
		set_TTY_color(is_colorized, os, context.isColorSet()? static_cast<TTYColor>(context.color()): TTYColor::Brown) << "|M|";
		break;
	case NecroLog::Level::Debug:
		set_TTY_color(is_colorized, os, context.isColorSet()? static_cast<TTYColor>(context.color()): TTYColor::LightGray) << "|D|";
		break;
	default:
		set_TTY_color(is_colorized, os, context.isColorSet()? static_cast<TTYColor>(context.color()): TTYColor::LightRed) << "|?|";
		break;
	}
	os << " ";

	os << msg;

	if(is_colorized)
		os << "\33[0m";

	os << std::endl;
	os.flush();
}

