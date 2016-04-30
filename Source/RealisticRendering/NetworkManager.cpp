// Fill out your copyright notice in the Description page of Project Settings.

#include "RealisticRendering.h"
#include "NetworkManager.h"
#include "Networking.h"
#include <string>

UNetworkManager::UNetworkManager()
{
	ConnectionSocket = NULL;
	Listener = NULL;
	World = NULL;
	WorldTimerManager = NULL;
	bIsConnected = false;
	// TODO: Check unexpected client disconnection
}

UNetworkManager::~UNetworkManager()
{
}

void UNetworkManager::ListenSocket()
{
	UE_LOG(LogTemp, Warning, TEXT("Start Listening a Socket"));
	// FString IPAddress = "127.0.0.1";
	// FIPv4Address IPAddress = FIPv4Address(127, 0, 0, 1);
	FIPv4Address IPAddress = FIPv4Address(0, 0, 0, 0);
	FIPv4Endpoint Endpoint(IPAddress, PortNum);
	// FSocket*
	Listener = FTcpSocketBuilder(TEXT("UnrealCV Controller"))
		.AsReusable().BoundToEndpoint(Endpoint)
		.Listening(1); // Only accept one connection

	// When will happen if the endpoint is taken?

	int32 BufferSize = 2 * 1024 * 1024, RealSize;
	Listener->SetReceiveBufferSize(BufferSize, RealSize); // RealSize might be different from what we want
	check(RealSize == BufferSize);

	check(Listener); // TODO: Better exception handling

	check(World);

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle, this,
		&UNetworkManager::WaitConnection, 1, true);
}

void UNetworkManager::WaitConnection()
{
	static int NumAttempt = 0;
	if (!Listener) return; // In case of Socket initialization failed
	// check(Listener);
	NumAttempt += 1;
	UE_LOG(LogTemp, Warning, TEXT("Check connection %d"), NumAttempt);


	TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr(); // Why use TSharedRef? For thread safety?
	bool Pending;

	if (Listener->HasPendingConnection(Pending) && Pending)
	{
		check(ConnectionSocket == NULL); // Assume only one connection
		ConnectionSocket = Listener->Accept(*RemoteAddress, TEXT("Remote socket to send message back"));

		bIsConnected = true;
		UE_LOG(LogTemp, Warning, TEXT("Client socket connected."));

		if (ConnectionSocket != NULL)
		{
			FTimerHandle TimerHandle;
			World->GetTimerManager().SetTimer(TimerHandle, this, &UNetworkManager::WaitData, 1, true);
		}
	}
}


void UNetworkManager::WaitData()
{
	if (!ConnectionSocket) return;

	TArray<uint8> ReceivedData;

	uint32 Size, MaxSize = 65507u;
	while (ConnectionSocket->HasPendingData(Size))
	{
		// ReceivedData.Init(FMath::Min(Size, MaxSize)); // TODO: Check this magic number
		ReceivedData.SetNumUninitialized(FMath::Min(Size, MaxSize));

		int32 Read = 0;
		ConnectionSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read); // TODO: Check Robustness.
		// TODO: Define data exchange format? Maybe protobuf?
	}

	if (ReceivedData.Num() <= 0)
	{
		return;
	}
	const FString ReceivedString = StringFromBinaryArray(ReceivedData);

	UE_LOG(LogTemp, Warning, TEXT("%s"), *ReceivedString);
}

FString UNetworkManager::StringFromBinaryArray(const TArray<uint8>& BinaryArray)
{
	//Create a string from a byte array!
	std::string cstr(reinterpret_cast<const char*>(BinaryArray.GetData()), BinaryArray.Num());
	return FString(cstr.c_str());
}

void UNetworkManager::SendMessage(FString Message)
{
	UE_LOG(LogTemp, Warning, TEXT("Message to client: %s"), *Message);
	if (bIsConnected && ConnectionSocket)
	{
		TCHAR* SerializedChar = Message.GetCharArray().GetData();
		int32 Size = FCString::Strlen(SerializedChar);
		int32 Sent = 0;
		ConnectionSocket->Send((uint8*)TCHAR_TO_UTF8(SerializedChar), Size, Sent);
	}
}
