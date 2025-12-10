#pragma once

#include "necrologglobal.h"
#include "necrologlevel.h"

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <sstream>
#include <functional>

class NECROLOG_DECL_EXPORT NecroLog
{
	friend class NecroLogSharedData;
public:
	using Level = NecroLogLevel;
	enum class Color : int8_t {Default = -1, Black, Red, Green, Brown, Blue, Magenta, Cyan, LightGray, Gray, LightRed, LightGreen, Yellow, LightBlue, LightMagenta, LightCyan, White};
	class NECROLOG_DECL_EXPORT LogContext
	{
	public:
		LogContext() = default;
		LogContext(const char *file_name, int line_number, const char *topic)
			: m_topic(topic), m_file(file_name), m_line(line_number) {}

		const char *topic() const {return m_topic? m_topic: "";}
		const char *file() const {return m_file? m_file: "";}
		int line() const {return m_line;}

		bool isColorSet() const {return m_color != NecroLog::Color::Default;}
		NecroLog::Color color() const {return m_color;}
		void setColor(NecroLog::Color c) {m_color = c;}

		bool isTopicSet() const {return topic()[0];}
		//bool isFileSet() const {return file()[0];}
	private:
		const char *m_topic = nullptr;
		const char *m_file = nullptr;
		int m_line = 0;
		NecroLog::Color m_color = NecroLog::Color::Default;
	};

	static const char* levelToString(Level level);
	static Level stringToLevel(const char *str);

	using MessageHandler = std::function<void (Level level, const LogContext &context, const std::string &msg)>;
public:
	NecroLog(Level level, const LogContext& log_context)
	{
		if(level > Level::Invalid)
			m_necro = std::make_shared<NecroLogSharedData>(level, log_context);
	}

	template<typename T>
	NecroLog& operator<<(const T &v) {if(m_necro) *m_necro << v; return *this;}
	NecroLog& operator<<(bool b) {if(m_necro) *m_necro << (b? "true": "false"); return *this;}
	NecroLog& nospace() {if(m_necro) m_necro->setSpace(false); return *this;}
	NecroLog& color(NecroLog::Color c) {if(m_necro) m_necro->setColor(c); return *this;}

	static NecroLog create(Level level, const LogContext &log_context);
	static bool shouldLog(Level level, const LogContext &context);

	static MessageHandler setMessageHandler(MessageHandler h);

	static void defaultMessageHandler(Level level, const LogContext &context, const std::string &msg);
	static void writeWithDefaultFormat(std::ostream &os, bool is_colorized, Level level, const LogContext &context, const std::string &msg);
	static std::string moduleFromFileName(const char *file_name);

	static std::vector<std::string> setCLIOptions(int argc, char *argv[]);
	static std::vector<std::string> setCLIOptions(const std::vector<std::string> &params);
	static std::string thresholdsLogInfo();
	static std::string topicsLogThresholds();
	static void setTopicsLogThresholds(const std::string &thresholds);
	static void registerTopic(const std::string &topic, const std::string &info);
	static std::string registeredTopicsInfo();
	static const char * cliHelp();

	enum class ColorizedOutputMode {IfTty, Yes, No };
	static ColorizedOutputMode colorizedOutputMode();
	static void setColorizedOutputMode(ColorizedOutputMode mode);
private:
	struct Options
	{
		//std::map<std::string, Level> fileTresholds;
		std::map<std::string, Level> topicThresholds;
		std::map<std::string, std::string> registeredTopics;
		bool logLongFileNames = false;
		bool caseInsensitiveTopicMatch = false;
		ColorizedOutputMode colorizedOutput = ColorizedOutputMode::IfTty;
	};

	static Options& globalOptions();
	static int moduleNameStart(const char *file_name);
private:
	class NECROLOG_DECL_EXPORT NecroLogSharedData {
		friend class NecroLog;
	public:
		NecroLogSharedData(NecroLog::Level level, const LogContext& log_context);
		~NecroLogSharedData();

		void setSpace(bool b) {m_isSpace = b;}
		void setColor(NecroLog::Color c) {m_logContext.setColor(c);}

		template<typename T>
		void operator<<(const T &v) {if(m_level != NecroLog::Level::Invalid) {maybeSpace(); m_os << v;}}
	private:
		void maybeSpace() { if(m_isSpace && m_os.tellp() > 0) m_os << ' '; }
	private:
		std::ostringstream m_os;
		NecroLog::Level m_level;
		LogContext m_logContext;
		bool m_isSpace = true;
	};
private:
	std::shared_ptr<NecroLogSharedData> m_necro;
};

//template<> inline NecroLog& NecroLog::operator<<(const bool &b) {if(m_necro) *m_necro << (b? "true": "false"); return *this;}

#ifdef NECROLOG_NO_DEBUG_LOG
#define nCDebug(topic) while(0) NecroLog::create(NecroLog::Level::Debug, NecroLog::LogContext(__FILE__, __LINE__, topic))
#else
#define nCDebug(topic) for(bool necrolog__en__ = NecroLog::shouldLog(NecroLog::Level::Debug, NecroLog::LogContext(__FILE__, __LINE__, topic)); necrolog__en__; necrolog__en__ = false) NecroLog::create(NecroLog::Level::Debug, NecroLog::LogContext(__FILE__, __LINE__, topic))
#endif
#define nCInfo(topic) for(bool necrolog__en__ = NecroLog::shouldLog(NecroLog::Level::Info, NecroLog::LogContext(__FILE__, __LINE__, topic)); necrolog__en__; necrolog__en__ = false) NecroLog::create(NecroLog::Level::Info, NecroLog::LogContext(__FILE__, __LINE__, topic))
#define nCMessage(topic) for(bool necrolog__en__ = NecroLog::shouldLog(NecroLog::Level::Message, NecroLog::LogContext(__FILE__, __LINE__, topic)); necrolog__en__; necrolog__en__ = false) NecroLog::create(NecroLog::Level::Message, NecroLog::LogContext(__FILE__, __LINE__, topic))
#define nCWarning(topic) for(bool necrolog__en__ = NecroLog::shouldLog(NecroLog::Level::Warning, NecroLog::LogContext(__FILE__, __LINE__, topic)); necrolog__en__; necrolog__en__ = false) NecroLog::create(NecroLog::Level::Warning, NecroLog::LogContext(__FILE__, __LINE__, topic))
#define nCError(topic) for(bool necrolog__en__ = NecroLog::shouldLog(NecroLog::Level::Error, NecroLog::LogContext(__FILE__, __LINE__, topic)); necrolog__en__; necrolog__en__ = false) NecroLog::create(NecroLog::Level::Error, NecroLog::LogContext(__FILE__, __LINE__, topic))

#define nDebug() nCDebug("")
#define nMessage() nCMessage("")
#define nInfo() nCInfo("")
#define nWarning() nCWarning("")
#define nError() nCError("")

#ifdef NECROLOG_NO_DEBUG_LOG

#define nLogScope(name) while(0) nDebug()
#define nLogFuncFrame() while(0) nDebug()

#else

#define nLogScope(name) NecroLog __necrolog_func_frame_exit_logger__ = NecroLog::shouldLog(NecroLog::Level::Debug, NecroLog::LogContext(__FILE__, __LINE__, ""))? \
NecroLog::create(NecroLog::Level::Debug, NecroLog::LogContext(__FILE__, __LINE__, "")) << "<< EXIT scope" << name : \
NecroLog::create(NecroLog::Level::Invalid, NecroLog::LogContext(__FILE__, __LINE__, "")); \
nDebug() << ">> ENTER scope" << name

#define nLogFuncFrame() NecroLog __necrolog_func_frame_exit_logger__ = NecroLog::shouldLog(NecroLog::Level::Debug, NecroLog::LogContext(__FILE__, __LINE__, ""))? \
NecroLog::create(NecroLog::Level::Debug, NecroLog::LogContext(__FILE__, __LINE__, "")) << "     EXIT FN" << __FUNCTION__ : \
NecroLog::create(NecroLog::Level::Invalid, NecroLog::LogContext(__FILE__, __LINE__, "")); \
nDebug() << ">>>> ENTER FN" << __FUNCTION__

#endif
