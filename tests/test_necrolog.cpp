#include <necrolog.h>

#include <numeric>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <iostream>
#include <cassert>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace std;

static void log_case(const string &args, const map<const char*, NecroLogLevel> &expected_levels)
{
	istringstream iss(args);
	vector<string> params{istream_iterator<string>{iss}, istream_iterator<string>{}};
	NecroLog::setCLIOptions(params);
	for(const auto &[topic, max_level] : expected_levels) {
		for(auto level : {NecroLogLevel::Debug, NecroLogLevel::Message, NecroLogLevel::Info, NecroLogLevel::Warning, NecroLogLevel::Error}) {
			auto will_log = NecroLog::shouldLog(level, NecroLog::LogContext(__FILE__, __LINE__, topic));
			bool should_log = level <= max_level;
			auto level_str = string(NecroLog::levelToString(level));
			auto max_level_str = string(NecroLog::levelToString(max_level));
			auto logged_level_str = string(NecroLog::levelToString(level));
			auto topic_str = string(topic);
			CAPTURE(args);
			CAPTURE(topic_str);
			CAPTURE(level_str);
			CAPTURE(max_level_str);
			CAPTURE(logged_level_str);
			REQUIRE(should_log == will_log);
		}
	}
}

static tuple<string, map<const char*, NecroLogLevel>> make_args(const string &args, const map<const char*, NecroLogLevel> &expected_levels)
{
	return make_tuple(args, expected_levels);
}

DOCTEST_TEST_CASE("CLI thresholds")
{
	for(const auto &[args, expected_levels] : {
		make_args("myapp ", {{"", NecroLogLevel::Info}, {"foo", NecroLogLevel::Warning}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp -v", {{"", NecroLogLevel::Debug}, {"foo", NecroLogLevel::Debug}, {"bar", NecroLogLevel::Debug}}),
		make_args("myapp -v :D", {{"", NecroLogLevel::Debug}, {"foo", NecroLogLevel::Debug}, {"bar", NecroLogLevel::Debug}}),
		make_args("myapp -v :M", {{"", NecroLogLevel::Message}, {"foo", NecroLogLevel::Message}, {"bar", NecroLogLevel::Message}}),
		make_args("myapp -v :I", {{"", NecroLogLevel::Info}, {"foo", NecroLogLevel::Info}, {"bar", NecroLogLevel::Info}}),
		make_args("myapp -v :W", {{"", NecroLogLevel::Warning}, {"foo", NecroLogLevel::Warning}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp -v :E", {{"", NecroLogLevel::Error}, {"foo", NecroLogLevel::Error}, {"bar", NecroLogLevel::Error}}),
		make_args("myapp -v foo:D", {{"", NecroLogLevel::Info}, {"foo", NecroLogLevel::Debug}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp -v foo:M", {{"", NecroLogLevel::Info}, {"foo", NecroLogLevel::Message}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp -v foo:I", {{"", NecroLogLevel::Info}, {"foo", NecroLogLevel::Info}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp -v foo:W", {{"", NecroLogLevel::Info}, {"foo", NecroLogLevel::Warning}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp -v foo:E", {{"", NecroLogLevel::Info}, {"foo", NecroLogLevel::Error}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp -v test_necr:D", {{"", NecroLogLevel::Debug}, {"foo", NecroLogLevel::Warning}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp -v test_necr:M", {{"", NecroLogLevel::Message}, {"foo", NecroLogLevel::Warning}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp -v test_necr:I", {{"", NecroLogLevel::Info}, {"foo", NecroLogLevel::Warning}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp -v test_necr:W", {{"", NecroLogLevel::Warning}, {"foo", NecroLogLevel::Warning}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp -v test_necr:E", {{"", NecroLogLevel::Error}, {"foo", NecroLogLevel::Warning}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp -v foo,bar:W", {{"", NecroLogLevel::Info}, {"foo", NecroLogLevel::Debug}, {"bar", NecroLogLevel::Warning}}),
		make_args("myapp --vi Foo:D", {{"", NecroLogLevel::Info}, {"foo", NecroLogLevel::Debug}, {"bar", NecroLogLevel::Warning}}),
	}) {
		log_case(args, expected_levels);
	}
}
