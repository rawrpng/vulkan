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


class netclient
{
public:
	enum class ConnectionStatus
	{
		Disconnected = 0, Connected, Connecting, FailedToConnect
	};
public:
	using DataReceivedCallback = std::function<void(const netbuffer)>;
	using ServerConnectedCallback = std::function<void()>;
	using ServerDisconnectedCallback = std::function<void()>;
public:
	netclient() = default;
	~netclient();

	void ConnectToServer(const std::string& serverAddress);
	void Disconnect();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Set callbacks for server events
	// These callbacks will be called from the network thread
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SetDataReceivedCallback(const DataReceivedCallback& function);
	void SetServerConnectedCallback(const ServerConnectedCallback& function);
	void SetServerDisconnectedCallback(const ServerDisconnectedCallback& function);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Send Data
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void SendBuffer(netbuffer buffer, bool reliable = true);
	void SendString(std::string string, bool reliable = true);
	void sendgamestate(int state, bool reliable = true);
	void sendgamepos(const glm::vec3& pos, bool reliable = true);


	template<typename T>
	void SendData(const T& data, bool reliable = true)
	{
		SendBuffer(Buffer(&data, sizeof(T)), reliable);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Connection Status & Debugging
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool IsRunning() const { return m_Running; }
	ConnectionStatus GetConnectionStatus() const { return m_ConnectionStatus; }
	const std::string& GetConnectionDebugMessage() const { return m_ConnectionDebugMessage; }
private:
	void NetworkThreadFunc();
	void Shutdown();
private:
	static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* info);
	void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

	void PollIncomingMessages();
	void PollConnectionStateChanges();

	void OnFatalError(const std::string& message);
private:
	std::thread m_NetworkThread;
	DataReceivedCallback m_DataReceivedCallback;
	ServerConnectedCallback m_ServerConnectedCallback;
	ServerDisconnectedCallback m_ServerDisconnectedCallback;

	ConnectionStatus m_ConnectionStatus = ConnectionStatus::Disconnected;
	std::string m_ConnectionDebugMessage;

	std::string m_ServerAddress;
	bool m_Running = false;

	ISteamNetworkingSockets* m_Interface = nullptr;
	HSteamNetConnection m_Connection = 0;
};
