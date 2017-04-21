// Copyright 2016 wang jie(newzeadev@gmail.com). All Rights Reserved.

#include "RakNetPrivatePCH.h"
#include "RakNetUDPClient.h"

#include "MessageIdentifiers.h"
#include "PacketLogger.h"

enum class CustomMessageIDTypes
{
	StartOffset = ID_USER_PACKET_ENUM,
	IDRakNetCustomData,
	IDRakNetCustomCompressFlag,
};

URakNetUDPClient::URakNetUDPClient()
{
	UDPConnection = RakNet::RakPeerInterface::GetInstance();
	if (UDPConnection)
	{
		UDPConnection->SetIncomingPacketEventHandler(
			std::bind(&URakNetUDPClient::OnIncomingPacket, this, std::placeholders::_1));
	}

	NetThreadPriority = 0;
	ShutdownDelayMS = 500;

	DefaultPriority = UDPPacketPriority::ImmediatePriority;
	DefaultReliablitity = UDPPacketReliability::ReliableOrdered;
}

UDPConnectionAttemptResult URakNetUDPClient::Connect(const FString& Host, int32 Port)
{
	using namespace RakNet;

	if (UDPConnection)
	{
		if (!UDPConnection->IsActive())
		{
			SocketDescriptor SocketDesc;

			UE_LOG(LogRakNet, Display, TEXT("RakNet startup start."));

			const StartupResult Result = UDPConnection->Startup(1, &SocketDesc, 1, NetThreadPriority);
			if (Result != RAKNET_STARTED && Result != RAKNET_ALREADY_STARTED)
			{
				UE_LOG(LogRakNet, Error,
					TEXT("RakNet startup failed! Error[%d]! Host[%s], Port[%d]."),
					(int32)Result, *Host, Port);

				return UDPConnectionAttemptResult::ConnectionStartupFailed;
			}

			UDPConnection->SetOccasionalPing(true);
			UDPConnection->SetUnreliableTimeout(1000);

			UE_LOG(LogRakNet, Display, TEXT("RakNet startup finish."));
		}

		const ConnectionAttemptResult AttemptResult = UDPConnection->Connect(TCHAR_TO_ANSI(*Host), Port, NULL, 0);
		switch (AttemptResult)
		{
		case CONNECTION_ATTEMPT_STARTED:
			UE_LOG(LogRakNet, Display, TEXT("RakNet start connection to Host[%s], Port[%d]."), *Host, Port);
			return UDPConnectionAttemptResult::ConnectionAttemptStarted;

		case INVALID_PARAMETER:
			UE_LOG(LogRakNet, Error, TEXT("RakNet connect failed! Invalid Parameter! Host[%s], Port[%d]."), *Host, Port);
			return UDPConnectionAttemptResult::InvalidParameter;

		case CANNOT_RESOLVE_DOMAIN_NAME:
			UE_LOG(LogRakNet, Error, TEXT("RakNet connect failed! Cannot resolve domain name! Host[%s], Port[%d]."), *Host, Port);
			return UDPConnectionAttemptResult::CannotResolveDomainName;

		case ALREADY_CONNECTED_TO_ENDPOINT:
			UE_LOG(LogRakNet, Display, TEXT("RakNet already connected to Host[%s], Port[%d]."), *Host, Port);
			return UDPConnectionAttemptResult::AlreadyConnectedToEndpoint;

		case CONNECTION_ATTEMPT_ALREADY_IN_PROGRESS:
			UE_LOG(LogRakNet, Display, TEXT("RakNet connection attempt already in progress! Host[%s], Port[%d]."), *Host, Port);
			return UDPConnectionAttemptResult::ConnectionAttemptAlreadyInProgress;

		case SECURITY_INITIALIZATION_FAILED:
			UE_LOG(LogRakNet, Error, TEXT("RakNet connect failed! Security Initialization failed! Host[%s], Port[%d]."), *Host, Port);
			return UDPConnectionAttemptResult::SecurityInitializationFailed;

		default:
			UE_LOG(LogRakNet, Error, TEXT("RakNet connect failed! Unknown Error[%d]!"), (int32)AttemptResult);
			return UDPConnectionAttemptResult::UnknownError;
		}
	}
	else
	{
		UE_LOG(LogRakNet, Error,
			TEXT("RakNet connect failed! RakNet client initialize failed! Host[%s], Port[%d]."), *Host, Port);

		return UDPConnectionAttemptResult::InvalidInterfaceInstance;
	}
}

int32 URakNetUDPClient::SendWithOption(const TArray<uint8>& Message, UDPPacketPriority Priority, UDPPacketReliability Reliability)
{
	return 0;
}

int32 URakNetUDPClient::Send(const TArray<uint8>& Message)
{
	return SendWithOption(Message, DefaultPriority, DefaultReliablitity);
}

bool URakNetUDPClient::IsActive() const
{
	return UDPConnection ? UDPConnection->IsActive() : false;
}

void URakNetUDPClient::BeginDestroy()
{
	UE_LOG(LogRakNet, Display, TEXT("RakNet destroy begin."));

	if (UDPConnection)
	{
		UDPConnection->Shutdown(ShutdownDelayMS);
		UDPConnection->SetIncomingPacketEventHandler(nullptr);

		RakNet::RakPeerInterface::DestroyInstance(UDPConnection);
	}

	UE_LOG(LogRakNet, Display, TEXT("RakNet destroy finish."));

	UDPConnection = nullptr;

	Super::BeginDestroy();
}

bool URakNetUDPClient::OnIncomingPacket(const RakNet::Packet* InPacket)
{
	if (!InPacket || InPacket->bitSize <= 0)
	{
		UE_LOG(LogRakNet, Error, TEXT("RakNet received an invalid message!"));
		return false;
	}

	const FString Host = ANSI_TO_TCHAR(InPacket->systemAddress.ToString(false));
	const int32 Port = InPacket->systemAddress.GetPort();
	const uint32 Guid = RakNet::RakNetGUID::ToUint32(InPacket->guid);
	const int32 MessageType = InPacket->data[0];
	bool MessageHandled = true;

	switch (MessageType)
	{
	case ID_DISCONNECTION_NOTIFICATION:
		UE_LOG(LogRakNet, Error, TEXT("RakNet connection closed by remote! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionClosed.Broadcast(Host, Port, Guid, UDPConnectionLostReason::ClosedByRemote);
		});
		break;

	case ID_CONNECTION_LOST:
		UE_LOG(LogRakNet, Error, TEXT("RakNet connection lost! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionClosed.Broadcast(Host, Port, Guid, UDPConnectionLostReason::ConnectionLost);
		});
		break;

	case ID_CONNECTION_REQUEST_ACCEPTED:
		UE_LOG(LogRakNet, Display, TEXT("RakNet connection opened! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionOpened.Broadcast(Host, Port, Guid);
		});
		break;

	case ID_CONNECTION_ATTEMPT_FAILED:
		UE_LOG(LogRakNet, Error, TEXT("RakNet connection attempt failed! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionAttemptFailed.Broadcast(Host, Port, Guid, ConnectionAttemptFailed);
		});
		break;

	case ID_REMOTE_SYSTEM_REQUIRES_PUBLIC_KEY:
		UE_LOG(LogRakNet, Error,
			TEXT("RakNet connect failed! Remote system requires public key! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionAttemptFailed.Broadcast(Host, Port, Guid, RemoteSystemRequiresPublicKey);
		});
		break;

	case ID_OUR_SYSTEM_REQUIRES_SECURITY:
		UE_LOG(LogRakNet, Error,
			TEXT("RakNet connect failed! Our system requires security! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionAttemptFailed.Broadcast(Host, Port, Guid, OurSystemRequiresSecurity);
		});
		break;

	case ID_PUBLIC_KEY_MISMATCH:
		UE_LOG(LogRakNet, Error,
			TEXT("RakNet connect failed! Public key mismatch! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionAttemptFailed.Broadcast(Host, Port, Guid, PublicKeyMismatch);
		});
		break;

	case ID_ALREADY_CONNECTED:
		UE_LOG(LogRakNet, Warning, TEXT("RakNet connection already connected! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionAttemptFailed.Broadcast(Host, Port, Guid, AlreadyConnected);
		});
		break;

	case ID_NO_FREE_INCOMING_CONNECTIONS:
		UE_LOG(LogRakNet, Error,
			TEXT("RakNet connect failed! No free incoming connections! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionAttemptFailed.Broadcast(Host, Port, Guid, NoFreeIncomingConnections);
		});
		break;

	case ID_CONNECTION_BANNED:
		UE_LOG(LogRakNet, Error,
			TEXT("RakNet connect failed! Connection banned! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionAttemptFailed.Broadcast(Host, Port, Guid, ConnectionBanned);
		});
		break;

	case ID_INVALID_PASSWORD:
		UE_LOG(LogRakNet, Error,
			TEXT("RakNet connect failed! Invalid password! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionAttemptFailed.Broadcast(Host, Port, Guid, InvalidPassword);
		});
		break;

	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		UE_LOG(LogRakNet, Error,
			TEXT("RakNet connect failed! Incompatible protocol version! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionAttemptFailed.Broadcast(Host, Port, Guid, IncompatibleProtocol);
		});
		break;

	case ID_IP_RECENTLY_CONNECTED:
		UE_LOG(LogRakNet, Error,
			TEXT("RakNet connect failed! IP recently connected! Host[%s], Port[%d]."), *Host, Port);
		AsyncTask(ENamedThreads::GameThread, [this, Host, Port, Guid]()
		{
			OnConnectionAttemptFailed.Broadcast(Host, Port, Guid, IpRecentlyConnected);
		});
		break;

	case CustomMessageIDTypes::IDRakNetCustomData:
		break;

	case CustomMessageIDTypes::IDRakNetCustomCompressFlag:
		break;

	default:
		MessageHandled = false;
		UE_LOG(LogRakNet, Warning,
			TEXT("Unknown raknet message type! ID[%d][%s], Host[%s], Port[%d]."),
			MessageType, ANSI_TO_TCHAR(RakNet::PacketLogger::BaseIDTOString(MessageType)), *Host, Port);
		break;
	}

	return MessageHandled;
}

