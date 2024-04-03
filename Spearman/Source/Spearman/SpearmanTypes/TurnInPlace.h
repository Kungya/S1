#pragma once

UENUM(BlueprintType)
enum class ETurnInPlace : uint8
{
	ETIP_Left UMETA(DisplayName = "Turn Left"),
	ETIP_Right UMETA(DisplayName = "Turn Right"),
	ETIP_NotTurn UMETA(DisplayName = "Not Turn"),

	ETIP_MAX UMETA(DisplayName = "DefaultMax")
};