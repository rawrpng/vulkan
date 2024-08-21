#pragma once
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
#include "netbuffer.hpp"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#ifndef STEAMNETWORKINGSOCKETS_OPENSOURCE
#include <steam/steam_api.h>
#endif

#include <string>
#include <map>
#include <thread>
#include <functional>
#include <glm/glm.hpp>

using ClientID = HSteamNetConnection;

struct ClientInfo
{
	ClientID ID;
	std::string ConnectionDesc;
};

class netserver
{
public:
	using DataReceivedCallback = std::function<void(const ClientInfo&, const netbuffer)>;
	using ClientConnectedCallback = std::function<void(const ClientInfo&)>;
	using ClientDisconnectedCallback = std::function<void(const ClientInfo&)>;
public:
	netserver(int port);
	~netserver();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Start and Stop the server
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Start();
	void Stop();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Set callbacks for server events
	// These callbacks will be called from the server thread
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SetDataReceivedCallback(const DataReceivedCallback& function);
	void SetClientConnectedCallback(const ClientConnectedCallback& function);
	void SetClientDisconnectedCallback(const ClientDisconnectedCallback& function);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Send Data
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SendBufferToClient(ClientID clientID, netbuffer buffer, bool reliable = true);
	void SendBufferToAllClients(netbuffer buffer, ClientID excludeClientID = 0, bool reliable = true);

	void SendStringToClient(const ClientID clientID, ClientID clientID2, std::string string, bool reliable = true);
	void sendgamestate(const ClientID clientID, ClientID clientID2, int state, bool reliable = true);
	void sendgamepos(const ClientID clientID, ClientID clientID2, const glm::vec3& pos, bool reliable = true);
	void sendconnections(const ClientID clientID,const std::vector<ClientID>& clientIDs, bool reliable = true);
	void SendStringToAllClients(const std::string& string, ClientID excludeClientID = 0, bool reliable = true);

	template<typename T>
	void SendDataToClient(ClientID clientID, const T& data, bool reliable = true)
	{
		SendBufferToClient(clientID, Buffer(&data, sizeof(T)), reliable);
	}

	template<typename T>
	void SendDataToAllClients(const T& data, ClientID excludeClientID = 0, bool reliable = true)
	{
		SendBufferToAllClients(Buffer(&data, sizeof(T)), excludeClientID, reliable);
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void KickClient(ClientID clientID);

	bool IsRunning() const { return m_Running; }
	const std::map<HSteamNetConnection, ClientInfo>& GetConnectedClients() const { return m_ConnectedClients; }
private:
	void NetworkThreadFunc(); // Server thread

	static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info);
	void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

	// Server functionality
	void PollIncomingMessages();
	void SetClientNick(HSteamNetConnection hConn, const char* nick);
	void PollConnectionStateChanges();

	void OnFatalError(const std::string& message);
private:
	std::thread m_NetworkThread;
	DataReceivedCallback m_DataReceivedCallback;
	ClientConnectedCallback m_ClientConnectedCallback;
	ClientDisconnectedCallback m_ClientDisconnectedCallback;

	int m_Port = 0;
	bool m_Running = false;
	std::map<HSteamNetConnection, ClientInfo> m_ConnectedClients;

	ISteamNetworkingSockets* m_Interface = nullptr;
	HSteamListenSocket m_ListenSocket = 0u;
	HSteamNetPollGroup m_PollGroup = 0u;
};
