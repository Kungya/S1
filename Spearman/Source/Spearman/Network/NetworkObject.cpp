// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkObject.h"

UWorld* UNetworkObject::GetWorld() const
{
	if (const UObject* MyOuter = GetOuter())
	{
		return MyOuter->GetWorld();
	}

	return nullptr;
}

AActor* UNetworkObject::GetOwningActor() const
{
	return GetTypedOuter<AActor>();
}

int32 UNetworkObject::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	check(GetOuter());
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool UNetworkObject::CallRemoteFunction(UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack)
{ // Call "Remote" (aka, RPC) functions through the actors NetDriver
	check(!HasAnyFlags(RF_ClassDefaultObject));
	AActor* Owner = GetOwningActor();
	UNetDriver* NetDriver = Owner->GetNetDriver();
	if (NetDriver)
	{
		NetDriver->ProcessRemoteFunction(Owner, Function, Parms, OutParms, Stack, this);
		return true;
	}
	return false;
}

void UNetworkObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
}

void UNetworkObject::Destroy()
{ /* Server Only */
	if (IsValid(this))
	{
		if (GetOwningActor()->HasAuthority())
		{
			OnDestroyed();
			MarkAsGarbage();
		}
	}
}

void UNetworkObject::OnDestroyed()
{
	// Notify Owner etc.
}
