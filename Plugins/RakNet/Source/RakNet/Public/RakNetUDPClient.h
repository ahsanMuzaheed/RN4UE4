// Copyright 2016 wang jie(newzeadev@gmail.com). All Rights Reserved.

#pragma once

#include "RakPeerInterface.h"

#include "UObject/NoExportTypes.h"
#include "RakNetUDPClient.generated.h"

UENUM()
enum class UDPConnectionAttemptResult
{
	None,
	ConnectionAttemptStarted,
	InvalidParameter,
	CannotResolveDomainName,
	AlreadyConnectedToEndpoint,
	ConnectionAttemptAlreadyInProgress,
	SecurityInitializationFailed,
	ConnectionStartupFailed,
	InvalidInterfaceInstance,
	UnknownError,
};

UENUM()
enum class UDPPacketPriority
{
	/// The highest possible priority. These message trigger sends immediately, and are generally not buffered or aggregated into a single datagram.
	ImmediatePriority,

	/// For every 2 IMMEDIATE_PRIORITY messages, 1 HIGH_PRIORITY will be sent.
	/// Messages at this priority and lower are buffered to be sent in groups at 10 millisecond intervals to reduce UDP overhead and better measure congestion control. 
	HighPriority,

	/// For every 2 HIGH_PRIORITY messages, 1 MEDIUM_PRIORITY will be sent.
	/// Messages at this priority and lower are buffered to be sent in groups at 10 millisecond intervals to reduce UDP overhead and better measure congestion control. 
	MediumPriority,

	/// For every 2 MEDIUM_PRIORITY messages, 1 LOW_PRIORITY will be sent.
	/// Messages at this priority and lower are buffered to be sent in groups at 10 millisecond intervals to reduce UDP overhead and better measure congestion control. 
	LowPriority,
};

UENUM()
enum class UDPPacketReliability
{
	/// Same as regular UDP, except that it will also discard duplicate datagrams.  RakNet adds (6 to 17) + 21 bits of overhead, 16 of which is used to detect duplicate packets and 6 to 17 of which is used for message length.
	Unreliable,

	/// Regular UDP with a sequence counter.  Out of order messages will be discarded.
	/// Sequenced and ordered messages sent on the same channel will arrive in the order sent.
	UnreliableSequenced,

	/// The message is sent reliably, but not necessarily in any order.  Same overhead as UNRELIABLE.
	Reliable,

	/// This message is reliable and will arrive in the order you sent it.  Messages will be delayed while waiting for out of order messages.  Same overhead as UNRELIABLE_SEQUENCED.
	/// Sequenced and ordered messages sent on the same channel will arrive in the order sent.
	ReliableOrdered,

	/// This message is reliable and will arrive in the sequence you sent it.  Out or order messages will be dropped.  Same overhead as UNRELIABLE_SEQUENCED.
	/// Sequenced and ordered messages sent on the same channel will arrive in the order sent.
	ReliableSequenced,
};

UENUM()
enum UDPConnectionLostReason
{
	/// Called RakPeer::CloseConnection()
	ClosedByUser,

	/// Got ID_DISCONNECTION_NOTIFICATION
	ClosedByRemote,

	/// GOT ID_CONNECTION_LOST
	ConnectionLost,
};

UENUM()
enum UDPConnectionAttemptFailedReason
{
	ConnectionAttemptFailed,
	AlreadyConnected,
	NoFreeIncomingConnections,
	SecurityPublicKeyMismatch,
	ConnectionBanned,
	InvalidPassword,
	IncompatibleProtocol,
	IpRecentlyConnected,
	RemoteSystemRequiresPublicKey,
	OurSystemRequiresSecurity,
	PublicKeyMismatch,
};

/**
 * 
 */
UCLASS()
class RAKNET_API URakNetUDPClient : public UObject
{
	GENERATED_BODY()
	
public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnConnectionOpened, FString, Host, int32, Port, uint32, Guid);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnConnectionClosed, FString, Host, int32, Port, uint32, Guid, UDPConnectionLostReason, Reason);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnConnectionAttemptFailed, FString, Host, int32, Port, uint32, Guid, UDPConnectionAttemptFailedReason, Reason);

	URakNetUDPClient();

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	UDPConnectionAttemptResult Connect(const FString& Host, int32 Port);

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	int32 SendWithOption(const TArray<uint8>& Message, UDPPacketPriority Priority, UDPPacketReliability Reliability);

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	int32 Send(const TArray<uint8>& Message);

	UFUNCTION(BlueprintCallable, Category = UDPClient)
	bool IsActive() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = UDPClient)
	int32 NetThreadPriority;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = UDPClient)
	int32 ShutdownDelayMS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UDPClient)
	UDPPacketPriority DefaultPriority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = UDPClient)
	UDPPacketReliability DefaultReliablitity;

	UPROPERTY(BlueprintAssignable, Category = "UDPClient|Event")
	FOnConnectionClosed OnConnectionClosed;

	UPROPERTY(BlueprintAssignable, Category = "UDPClient|Event")
	FOnConnectionOpened OnConnectionOpened;

	UPROPERTY(BlueprintAssignable, Category = "UDPClient|Event")
	FOnConnectionAttemptFailed OnConnectionAttemptFailed;

protected:
	virtual void BeginDestroy() override;

	virtual bool OnIncomingPacket(const RakNet::Packet* InPacket);

private:
	RakNet::RakPeerInterface* UDPConnection{ nullptr };
};
