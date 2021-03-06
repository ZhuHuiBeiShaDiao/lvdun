#ifndef FILTERMANAGER_H
#define FILTERMANAGER_H
#include <vector>
#include <string>

#include "boost/unordered_map.hpp"
#include "boost/unordered_set.hpp"

#include "CParseUrl.h"

#define FILTER_TYPE_SCRIPT 0x0001
#define FILTER_TYPE_IMAGE 0X0002
#define FILTER_TYPE_BACKGROUND 0x0004
#define FILTER_TYPE_STYLESHEET 0X0008
#define FILTER_TYPE_OBJECT 0X0010
#define FILTER_TYPE_XBL 0X0020
#define FILTER_TYPE_PING 0X0040
#define FILTER_TYPE_XMLHTTPREQUEST 0x0080
#define FILTER_TYPE_OBJECT_SUBREQUEST 0X0100
#define FILTER_TYPE_DTD 0X0200
#define FILTER_TYPE_SUBDOCUMENT 0X0400
#define FILTER_TYPE_DOCUMENT 0X0800
#define FILTER_TYPE_ELEMHIDE 0X1000
#define FILTER_TYPE_THIRD_PARTY 0x2000


#define  CFG_CHECK_VIDEO 0x0001
#define  CFG_CHECK_WHITE 0x0002
#define  CFG_CHECK_ALL   0xFFFF

#define  FILE_RULE_WEB 0x0001
#define  FILE_RULE_VIDEO 0x0002
#define  FILE_RULE_UESR 0x0003

typedef unsigned int FilterType;
typedef std::vector<std::string> StringVector;

class FilterRule;
class HideRule;
struct FilterRuleList;
struct HideRuleList;
class ReplaceRule;
struct ReplaceRuleList;

class RedirectRule;
struct RedirectRuleList;

typedef boost::unordered_map<std::string /*host*/, int /*state*/ > ConfigRuleMap;

class RuleMap
{
public:
	~RuleMap();
	typedef std::vector<FilterRule *> FilterRuleVector;
	class FilterRuleMap: public boost::unordered_map<std::string, FilterRuleList*> 
	{
		boost::unordered_set<unsigned int> unMatchRules;
	public:
		~FilterRuleMap();
		inline void prepareStartFind() {
			this->unMatchRules.clear();
		}

		int doFilter(const Url & mainURL, const std::string & key,const Url & url, FilterType t);
	};


	typedef boost::unordered_map<std::string, std::string> HideRuleMap;


	typedef std::vector<ReplaceRule *> ReplaceRuleVector;
	class ReplaceRuleMap: public boost::unordered_map<std::string, ReplaceRuleList*> 
	{
		boost::unordered_set<unsigned int> unMatchRules;
	public:
		~ReplaceRuleMap();
		//prepare to start find
		inline void prepareStartFind() {
			this->unMatchRules.clear();
		}

		void getReplace(const std::string &key,const Url & url,std::vector<std::string> &v_Parse);
	};
	
	typedef std::vector<RedirectRule *> RedirectRuleVector;
	class RedirectRuleMap: public boost::unordered_map<std::string, RedirectRuleList*> 
	{
		boost::unordered_set<unsigned int> unMatchRules;
	public:
		~RedirectRuleMap();
		//prepare to start find
		inline void prepareStartFind() {
			this->unMatchRules.clear();
		}

		bool shouldRedirect (const std::string &key,const Url &url,std::string &strRedirect);
	};
     
public:
	int shouldFilter(const Url & mainURL,const Url & url, FilterType t = 0);
	std::string getcssRules(const Url & url);
	void getreplaceRules(const Url & url,std::vector<std::string> & v_replace);
	bool shouldRedirect(const Url & mainURL,const Url & url, std::string& strRedirect);
	void addRule(FilterRule * r);
	void addRule(HideRule * r);
	void addRule(ReplaceRule * r);
	void addRule(RedirectRule * r);
protected:
	
	void insertRuleToFilterRuleMap(FilterRuleMap * rules,const std::string & key, FilterRule * r);
	void insertRuleToFilterRuleMap(ReplaceRuleMap * rules,const std::string & key, ReplaceRule * r);
	void insertRuleToFilterRuleMap(RedirectRuleMap * rules,const std::string & key, RedirectRule * r);

	int shouldFilterByShortcut(const Url & mainURL, const Url & url,FilterType t);
	int shouldFilterByDomain(const Url & mainURL, const Url & url,FilterType t,bool & isFind);
private:	
	//filter
	FilterRuleMap m_DomainWhiteRules; //white list use domain for map
	FilterRuleMap m_DomainFilterRules;

	FilterRuleMap m_ShortcutWhiteRules; //white list, use shortcut for map
	FilterRuleVector m_UnshortcutWhiteRules;

	FilterRuleMap m_ShortcutFilterRules;
	FilterRuleVector m_UnshortcutFilterRules;

	FilterRuleVector m_AllFilterRules;

	// hide
	HideRuleMap m_HideRules;

	//replace
	ReplaceRuleMap m_DomainReplaceRules;
	ReplaceRuleVector m_AllReplaceRules;

	//redirect
	RedirectRuleMap m_DomainRedirectRules;
	RedirectRuleVector m_AllRedirectRules;
};


class VideoRuleMap : public RuleMap
{
	
};

class FilterManager {

public:

	bool getWebRules(const std::wstring & filename);
	bool getVideoRules(const std::wstring & filename);
	bool getUsersRules(const std::wstring & filename);

	static FilterManager * getManager();

	bool updateConfigVideoHost(const Url & url,int istate = 0);
	bool updateConfigWhiteHost(const Url & url,bool bEnable);
	
	//先检查广告过滤配置 和 白名单里的

	int checkConfigHost(const Url & url,bool &bFind,std::string &strMatchHost,int cfgFlag = CFG_CHECK_ALL);


	//
	int shouldFilter(const Url & mainURL,const Url & url, FilterType t = 0);
	std::string getcssRules(const Url & url);
	void getreplaceRules(const Url & url,std::vector<std::string> & v_replace);

	bool shouldRedirect(const Url & mainURL,const Url & url, std::string& strRedirect);

private:

	RuleMap m_WebRules;
	RuleMap m_VideoRules;
	RuleMap m_UserRules;
	ConfigRuleMap m_ConfigWhiteRules;

private:
	//FilterManager(const std::wstring & filename);
	//FilterManager(const StringVector & rules);
private:
	bool getRules(const std::wstring & filename,int ruleType);

};
#endif // FILTERMANAGER_H
