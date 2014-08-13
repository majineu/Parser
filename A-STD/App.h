#pragma once
#include<map>
#include<string>
#include<ctime>

#include"Macros.h"
#include"SRParser.h"
#include"Scorer.h"



class CApp
{
public:
	CApp(void);
	~CApp(void);
	std::map<std::string, std::string> configMap;
	CSRParser *parser;
	
	void PrintConfig()
	{
		auto beg = configMap.begin(), end = configMap.end();
		for (; beg != end; ++beg)
			fprintf(stderr, "%-15s:%s\n", beg->first.c_str(), beg->second.c_str());
	}

public:
	bool ReadConfig(const char * pszConfig)
	{
		bool verbose = true;
		FILE * fp = fopen(pszConfig, "r");
		CHK_OPEN_FAILED(fp, pszConfig);
		const int maxLen = 65535;
		char buf[maxLen];

		/* read configuration */
		while (fgets(buf, maxLen, fp) != NULL)
		{
			if (strstr(buf, "##") != NULL)
				continue;

			/* skip empty line */
			removeNewLine(buf);
			if (strlen(buf) < 3)
				continue;

			char * key = strtok(buf, " \t");
			char * value = strtok(NULL, " \t");
			configMap[key] = value;
		}

		if (configMap.find("option") == configMap.end()	||
				configMap.find("template") == configMap.end()	||
				configMap.find("ignoreNull") == configMap.end()||
				configMap.find("sigEnd") == configMap.end()	||
				configMap.find("bs") == configMap.end())
				return false;

		parser = new CSRParser(atoi(configMap["bs"].c_str()), 
								configMap["option"] == std::string("featureCollection"));

		if (verbose == true)
		{
			map<std::string, std::string>::iterator it = configMap.begin(), end = configMap.end();
			for(; it != end; ++ it)
				fprintf(stderr, "%-15s  %-15s\n", it->first.c_str(), it->second.c_str());
		}

		fclose(fp);
		return true;
	}

	bool Run()
	{
		std::string opt = configMap["option"];
		CIDMap::ReadDepLabelFile("label.txt");


		if (opt == "training")
		{
			if (configMap.find("trainFile")   == configMap.end()|| 
					configMap.find("featureFile") == configMap.end()||
					configMap.find("iter")     == configMap.end()	||
					configMap.find("model")    == configMap.end()	||
					configMap.find("devFile")  == configMap.end()	||
					configMap.find("testFile") == configMap.end()	||
					configMap.find("nDelay")   == configMap.end()	||
					configMap.find("template") == configMap.end())
				return false;

			if (CIDMap::ScanningFeatures(configMap["featureFile"].c_str()) == false)
				return false;
			return parser->Training(configMap["trainFile"].c_str(),	   configMap["template"].c_str(),  
									configMap["devFile"].c_str(),      configMap["testFile"].c_str(), 
									atoi(configMap["iter"].c_str()),   configMap["model"].c_str(),
									atoi(configMap["nDelay"].c_str()));
		}
		else if (opt == "featureCollection")
		{
			if (configMap.find("trainFile")   == configMap.end() ||
					configMap.find("featureFile") == configMap.end() ||
					configMap.find("template")    == configMap.end() ||
					configMap.find("cutoff")      == configMap.end())
				return false;

			std::stringstream ss, ssCutoff;
			ss<<configMap["featureFile"] <<std::string("_cutoff_") <<configMap["cutoff"];
			ssCutoff << configMap["cutoff"];
			
			int cutoff;
			ssCutoff>>cutoff;
			parser->WriteFeatureFile(configMap["trainFile"].c_str(), ss.str().c_str(),
														   configMap["template"].c_str(), NULL, cutoff);
			return true;
		}
		else
		{
			fprintf(stderr, "Unknown option %s\n", configMap["option"].c_str());
			return false;
		}

		return true;
	}
};



