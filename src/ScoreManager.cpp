#include "global.h"
#include "RageLog.h"
#include "HighScore.h"
#include "GameConstantsAndTypes.h"
#include "Foreach.h"
#include "ScoreManager.h"
#include "XmlFile.h"
#include "XmlFileUtil.h"
#include "SongManager.h"

ScoreManager* SCOREMAN = NULL;

ScoreManager::ScoreManager() {
	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring(L, "SCOREMAN");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}
}

ScoreManager::~ScoreManager() {
	// Unregister with Lua.
	LUA->UnsetGlobal("SCOREMAN");
}




ScoresAtRate::ScoresAtRate() {
	bestGrade = Grade_Invalid;
	scores.clear();
	PBptr = nullptr;
}

void ScoresAtRate::AddScore(HighScore& hs) {
	RString& key = hs.GetScoreKey();	
	bestGrade = min(hs.GetWifeGrade(), bestGrade);
	scores.emplace(key, hs);

	if(!PBptr || PBptr->GetWifeScore() < hs.GetWifeScore())
		PBptr = &scores.find(key)->second;
}

vector<RString> ScoresAtRate::GetSortedKeys() {
	map<float, RString, greater<float>> tmp;
	vector<RString> o;
	FOREACHM(RString, HighScore, scores, i)
		 tmp.emplace(i->second.GetWifeScore(), i->first);
	FOREACHM(float, RString, tmp, j)
		 o.emplace_back(j->second);
	return o;
}

/*
const vector<HighScore*> ScoresAtRate::GetScores() const {
	map<float, RString, greater<float>> tmp;
	vector<HighScore*> o;
	FOREACHUM_CONST(RString, HighScore, scores, i)
		tmp.emplace(i->second.GetWifeScore(), i->first);
	FOREACHM(float, RString, tmp, j)
		o.emplace_back(j->second);
	return o;
}
*/

ScoresForChart::ScoresForChart() {
	bestGrade = Grade_Invalid;
	ScoresByRate.clear();
}

HighScore* ScoresForChart::GetPBAt(float& rate) {
	int key = RateToKey(rate);
	if(ScoresByRate.count(key))
		return ScoresByRate.at(key).PBptr;
	return NULL;
}

HighScore* ScoresForChart::GetPBUpTo(float& rate) {
	int key = RateToKey(rate);
	FOREACHM(int, ScoresAtRate, ScoresByRate, i) 
		if (i->first <= key)
			return i->second.PBptr;
		
	return NULL;
}

void ScoresForChart::AddScore(HighScore& hs) {
	bestGrade = min(hs.GetWifeGrade(), bestGrade);

	float rate = hs.GetMusicRate();
	int key = RateToKey(rate);
	ScoresByRate[key].AddScore(hs);
}

vector<float> ScoresForChart::GetPlayedRates() {
	vector<float> o;
	FOREACHM(int, ScoresAtRate, ScoresByRate, i)
		o.emplace_back(KeyToRate(i->first));
	return o;
}

vector<int> ScoresForChart::GetPlayedRateKeys() {
	vector<int> o;
	FOREACHM(int, ScoresAtRate, ScoresByRate, i)
		o.emplace_back(i->first);
	return o;
}

vector<RString> ScoresForChart::GetPlayedRateDisplayStrings() {
	vector<float> rates = GetPlayedRates();
	vector<RString> o;
	for(size_t i = 0; i < rates.size(); ++i)
		o.emplace_back(RateKeyToDisplayString(rates[i]));
	return o;
}

RString ScoresForChart::RateKeyToDisplayString(float rate) {
	RString rs = ssprintf("%.2f", rate);
	int j = 1;
	if (rs.find_last_not_of('0') == rs.find('.'))
		j = 2;
	rs.erase(rs.find_last_not_of('0') + j, rs.npos);
	rs.append("x");
	return rs;
}


vector<HighScore*> ScoresForChart::GetAllPBPtrs() {
	vector<HighScore*> o;
	FOREACHM(int, ScoresAtRate, ScoresByRate, i)
		o.emplace_back(i->second.PBptr);
	return o;
}

HighScore* ScoreManager::GetChartPBAt(RString& ck, float& rate) {
	if (pscores.count(ck))
		return pscores.at(ck).GetPBAt(rate);
	return NULL;
}

HighScore* ScoreManager::GetChartPBUpTo(RString& ck, float& rate) {
	if (pscores.count(ck))
		return pscores.at(ck).GetPBUpTo(rate);
	return NULL;
}







void ScoreManager::RecalculateSSRs() {
	return;
}

// should deal with this misnomer - mina
void ScoreManager::EnableAllScores() {
	return;
}

void ScoreManager::CalcPlayerRating(float& prating, float* pskillsets) {
	float skillsetsum = 0.f;
	FOREACH_ENUM(Skillset, ss) {
		// actually skip overall
		if (ss == Skill_Overall)
			continue;

		SortTopSSRPtrs(ss);
		pskillsets[ss] = AggregateSSRs(ss, 0.f, 10.24f, 1)*0.95f;
		CLAMP(pskillsets[ss], 0.f, 100.f);
		skillsetsum += pskillsets[ss];
	}

	prating = skillsetsum / (NUM_Skillset - 1);
}

// perhaps we will need a generalized version again someday, but not today
float ScoreManager::AggregateSSRs(Skillset ss, float rating, float res, int iter) const {
	double sum;
	do {
		rating += res;
		sum = 0.0;
		for (int i = 0; i < static_cast<int>(TopSSRs.size()); i++) {
			sum += max(0.0, 2.f / erfc(0.1*(TopSSRs[i]->GetSkillsetSSR(ss) - rating)) - 1.5);
		}
	} while (pow(2, rating * 0.1) < sum);
	if (iter == 11)
		return rating;
	return AggregateSSRs(ss, rating - res, res / 2.f, iter + 1);
}

void ScoreManager::SortTopSSRPtrs(Skillset ss) {
	TopSSRs.clear();
	FOREACHM(RString, ScoresForChart, pscores, i) {
		if (!SONGMAN->IsChartLoaded(i->first))
			continue;
		vector<HighScore*> pbs = i->second.GetAllPBPtrs();
		FOREACH(HighScore*, pbs, hs) {
			TopSSRs.emplace_back(*hs);
		}
	}

	auto ssrcomp = [&ss](HighScore* a, HighScore* b) { return (a->GetSkillsetSSR(ss) > b->GetSkillsetSSR(ss)); };
	sort(TopSSRs.begin(), TopSSRs.end(), ssrcomp);
}

HighScore* ScoreManager::GetTopSSRHighScore(unsigned int rank, int ss) {
	if (rank < 0)
		rank = 0;

	if (ss >= 0 && ss < NUM_Skillset && rank < TopSSRs.size())
		return TopSSRs[rank];

	return NULL;
}







// Write scores to xml
XNode* ScoresAtRate::CreateNode(const int& rate) const {
	XNode* o = new XNode("ScoresAt");

	RString rs = IntToString(rate);
	rs = rs.substr(0, 1) + "." + rs.substr(1, 3);
	// should be safe as this is only called if there is at least 1 score (which would be the pb)
	o->AppendAttr("PBKey", PBptr->GetScoreKey());
	o->AppendAttr("BestGrade", GradeToString(bestGrade));
	o->AppendAttr("Rate", rs);

	FOREACHM_CONST(RString, HighScore, scores, i)
		o->AppendChild(i->second.CreateEttNode());

	return o;
}

XNode * ScoresForChart::CreateNode(const RString& ck) const {
	XNode* o = new XNode("ChartScores");
	o->AppendAttr("Key", ck);

	FOREACHM_CONST(int, ScoresAtRate, ScoresByRate, i)
		o->AppendChild(i->second.CreateNode(i->first));

	return o;
}

XNode * ScoreManager::CreateNode() const {
	XNode* o = new XNode("ScoreManager");

	FOREACHM_CONST(RString, ScoresForChart, pscores, ch)
		o->AppendChild(ch->second.CreateNode(ch->first));

	return o;
}


// Read scores from xml
void ScoresAtRate::LoadFromNode(const XNode* node, const RString& ck, const float& rate) {
	RString sk;
	FOREACH_CONST_Child(node, p) {
		p->GetAttrValue("Key", sk);
		scores[sk].LoadFromEttNode(p);

		// Fill in stuff for the highscores
		scores[sk].SetChartKey(ck);
		scores[sk].SetScoreKey(sk);
		scores[sk].SetMusicRate(rate);
	}

	// Set the pbptr
	PBptr = &scores.find(sk)->second;
	bestGrade = PBptr->GetWifeGrade();
}

void ScoresForChart::LoadFromNode(const XNode* node, const RString& ck) {
	RString rs;
	int rate;
	FOREACH_CONST_Child(node, p) {
		ASSERT(p->GetName() == "ScoresAt");
		p->GetAttrValue("Rate", rs);
		rate = 10 * StringToInt(rs.substr(0, 1) + rs.substr(2, 4));
		ScoresByRate[rate].LoadFromNode(p, ck, KeyToRate(rate));
		bestGrade = ScoresByRate[rate].bestGrade;
	}
}

void ScoreManager::LoadFromNode(const XNode * node) {
	FOREACH_CONST_Child(node, p) {
		ASSERT(p->GetName() == "ChartScores");
		RString tmp;
		p->GetAttrValue("Key", tmp);
		const RString ck = tmp;
		pscores[ck].LoadFromNode(p, ck);
	}
}



ScoresAtRate* ScoresForChart::GetScoresAtRate(const int& rate) {
	auto it = ScoresByRate.find(rate);
	if (it != ScoresByRate.end())
		return &it->second;
	return NULL;
}

ScoresForChart* ScoreManager::GetScoresForChart(const RString& ck) {
	auto it = pscores.find(ck);
	if (it != pscores.end())
		return &it->second;
	return NULL;
}


#include "LuaBinding.h"

class LunaScoresAtRate: public Luna<ScoresAtRate>
{
public:
	static int GetScores(T* p, lua_State *L) {
		lua_newtable(L);
		vector<RString> keys = p->GetSortedKeys();
		for (size_t i = 0; i < keys.size(); ++i) {
			HighScore& wot = p->scores[keys[i]];
			wot.PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}

		return 1;
	}

	LunaScoresAtRate()
	{
		ADD_METHOD(GetScores);
	}
};

LUA_REGISTER_CLASS(ScoresAtRate)

class LunaScoresForChart : public Luna<ScoresForChart>
{
public:
	LunaScoresForChart()
	{
	}
};

LUA_REGISTER_CLASS(ScoresForChart)



class LunaScoreManager : public Luna<ScoreManager>
{
public:
	static int GetScoresByKey(T* p, lua_State *L) {
		const RString& ck = SArg(1);
		ScoresForChart* scores = p->GetScoresForChart(ck);

		if (scores) {
			lua_newtable(L);
			vector<int> ratekeys = scores->GetPlayedRateKeys();
			vector<RString> ratedisplay = scores->GetPlayedRateDisplayStrings();
			for (size_t i = 0; i < ratekeys.size(); ++i) {
				LuaHelpers::Push(L, ratedisplay[i]);
				scores->GetScoresAtRate(ratekeys[i])->PushSelf(L);
				lua_rawset(L, -3);
			}

			return 1;
		}

		lua_pushnil(L);
		return 1;
	}

	static int SortSSRs(T* p, lua_State *L) {
		p->SortTopSSRPtrs(Enum::Check<Skillset>(L, 1));
		return 1;
	}

	static int ValidateAllScores(T* p, lua_State *L) {
		p->EnableAllScores();
		return 1;
	}

	static int 	GetTopSSRHighScore(T* p, lua_State *L) {
		HighScore* ths = p->GetTopSSRHighScore(IArg(1) - 1, Enum::Check<Skillset>(L, 2));
		if (ths)
			ths->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	LunaScoreManager()
	{
		ADD_METHOD(GetScoresByKey);
		ADD_METHOD(SortSSRs);
		ADD_METHOD(ValidateAllScores);
		ADD_METHOD(GetTopSSRHighScore);
	}
};

LUA_REGISTER_CLASS(ScoreManager)