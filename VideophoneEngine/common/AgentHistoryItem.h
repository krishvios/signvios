#pragma once
#include <string>
#include <ctime>

struct AgentHistoryItem
{
	AgentHistoryItem (const std::string& agentId) :
		m_agentId(agentId)
	{
		time (&m_callJoinUtc);
	}

	AgentHistoryItem (const std::string& agentId, time_t callJoinUtc) :
		m_agentId (agentId),
		m_callJoinUtc (callJoinUtc) {}

	std::string m_agentId;
	time_t m_callJoinUtc{};
};
