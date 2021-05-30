#include "Sdk/Engine.hpp"

// C++ Headers
#include <thread>
#include <vector>
#include <string>
#include <iostream>

#undef min
#undef max

	/// Vector of all currently valid RappelComponents.
	/// NOTE: You gotta find a better way of clearing it of invalid
	/// pointers... mine is just an example.
std::vector<engine::RappelComponent*> rappelComponents {};

	/// REALLY ghetto, but it works!
bool recordedNewRappelComponents {};

[[nodiscard]] engine::RappelComponent *findClosestRappelComponent(const havok::Vector4f &origin)
{
	engine::RappelComponent *result {};

	for (auto minimumDistance = std::numeric_limits<float>::max(); 
		const auto &component: rappelComponents)
	{
		if (!component->actor())
			continue;

		const auto componentPosition = component->actor()->origin();
		if (const auto actorDistance = sqrtf(
			(componentPosition.x - origin.x) * (componentPosition.x - origin.x) +
			(componentPosition.y - origin.y) * (componentPosition.y - origin.y) +
			(componentPosition.z - origin.z) * (componentPosition.z - origin.z)
		); actorDistance < minimumDistance)
		{
			minimumDistance = actorDistance;
			result = component;
		}
	}

	return result;
}
[[nodiscard]] engine::RappelComponent *findFurthestRappelComponent(const havok::Vector4f &origin)
{
	engine::RappelComponent *result {};

	for (auto furthestDistance = 0.f;
		const auto &component : rappelComponents)
	{
		if (!component->actor())
			continue;

		const auto componentPosition = component->actor()->origin();
		if (const auto actorDistance = sqrtf(
			(componentPosition.x - origin.x) * (componentPosition.x - origin.x) +
			(componentPosition.y - origin.y) * (componentPosition.y - origin.y) +
			(componentPosition.z - origin.z) * (componentPosition.z - origin.z)
		); actorDistance > furthestDistance)
		{
			furthestDistance = actorDistance;
			result = component;
		}
	}

	return result;
}

namespace hooks
{
		/// Our hook on RappelComponent's constructor =D
	engine::RappelComponent*(*originalRappelComponentConstructor)(engine::RappelComponent*);
	engine::RappelComponent *hookedRappelComponentConstructor(engine::RappelComponent *instance)
	{
		originalRappelComponentConstructor(instance);
		rappelComponents.push_back(instance);
		recordedNewRappelComponents = true;
		
		std::printf("[INFO] Recorded RappelComponent: 0x%llX\n", reinterpret_cast<std::uintptr_t>(instance));
		return instance;
	}
}

unsigned long mainThread(const HMODULE moduleInstance)
{
	AllocConsole();
	freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
	freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);

	std::puts("-- R6 Rappel Fly Example --");

	// Hook the RappelComponent constructor.
	{
		hooks::originalRappelComponentConstructor = *reinterpret_cast<decltype(hooks::originalRappelComponentConstructor)*>(utilities::imageBase() + 0x702BA38);
		*reinterpret_cast<decltype(&hooks::hookedRappelComponentConstructor)*>(utilities::imageBase() + 0x702BA38) = hooks::hookedRappelComponentConstructor;
	}

	const auto gameManager = engine::GameManager::global();
	const auto roundManager = engine::RoundManager::global();

	const auto increment = 2.5f;
	auto vel = 100.f;
	auto mode = "closest";

	auto lastInput = VK_F4; // rapel up

	std::puts("[INFO] INSERT to exit...");
	while (!GetAsyncKeyState(VK_INSERT))
	{
		// We don't want to be too fast!
		std::this_thread::yield();

		// Fixing up [rappelComponents]
		if (!roundManager->isInGame() && recordedNewRappelComponents)
		{
			recordedNewRappelComponents = false;
			rappelComponents.clear();
			std::puts("[INFO] Reset list of RappelComponents!");
		}
		
		const auto localController = gameManager->getLocalController();
		if (!localController)
			continue;

		const auto localPawn = localController->getPawnComponent();
		if (!localPawn)
			continue;

		const auto localActor = localPawn->actor();
		if (!localActor)
			continue;

		const auto localInteractive = localPawn->interactive();
		if (!localInteractive)
			continue;

		const auto localInteractiveProperties = localPawn->interactiveProperties();
		if (!localInteractiveProperties)
			continue;

		const auto startPosition = localActor->origin();

		// While one of these keys is being held you can rappel specific directions
		if (GetAsyncKeyState(VK_F6))
		{
			std::puts("[INFO] Mode: RAPEL_X_DOWN");
			localInteractiveProperties->setRappelPosition({ startPosition.x - (increment * 2.f), startPosition.y, startPosition.z });
		} else if (GetAsyncKeyState(VK_F5))
		{
			std::puts("[INFO] Mode: RAPEL_X_UP");
			localInteractiveProperties->setRappelPosition({ startPosition.x + (increment * 2.f), startPosition.y, startPosition.z });
		} else if (GetAsyncKeyState(VK_F4))
		{
			std::puts("[INFO] Mode: RAPEL_UP");
			localInteractiveProperties->setRappelPosition({ startPosition.x, startPosition.y, startPosition.z + increment });
		} else if (GetAsyncKeyState(VK_F3))
		{
			std::puts("[INFO] Mode: RAPEL_DOWN");
			localInteractiveProperties->setRappelPosition({ startPosition.x, startPosition.y, startPosition.z - increment });
		} else if (GetAsyncKeyState(VK_F2))
		{
			std::puts("[INFO] Mode: RAPEL_Y_UP");
			localInteractiveProperties->setRappelPosition({ startPosition.x, startPosition.y + (increment * 2.f), startPosition.z });
		} else if (GetAsyncKeyState(VK_F1))
		{
			std::puts("[INFO] Mode: RAPEL_Y_DOWN");
			localInteractiveProperties->setRappelPosition({ startPosition.x, startPosition.y - (increment * 2.f), startPosition.z });
		} else
		{
			//std::puts("[INFO] Mode: DEFAULT");
			localInteractiveProperties->setRappelPosition({ startPosition.x, startPosition.y, startPosition.z });
		}

		if (GetAsyncKeyState(VK_F7) & 1)
		{
			if (vel == 100.f)
			{
				std::puts("[INFO] New veloctiy: 200");
				vel = 200.f;
			} else
			{
				std::puts("[INFO] New veloctiy: 100");
				vel = 100.f;
			}
			localInteractiveProperties->setRappelVelocity(vel);
		}

		if (GetAsyncKeyState(VK_F8) & 1)
		{
			if (mode == "closest")
			{
				std::puts("[INFO] Mode switched to furthest RappelComponent");
				mode = "furthest";
			} else
			{
				std::puts("[INFO] Mode switched to closest RappelComponent");
				mode = "closest";
			}
			
		}

		const auto localRappeller = localInteractive->getRappeller();
		if (!localRappeller)
			continue;

		// Use F9 to FLY!
		if (GetAsyncKeyState(VK_F9) & 1)
		{
			if (mode == "closest")
			{
				std::puts("[INFO] Flying to closest RappelComponent");
				if (const auto selection = findClosestRappelComponent(startPosition); selection)
					localRappeller->rappel(selection, &startPosition, 4);
			} else
			{
				std::puts("[INFO] Flying to furthest RappelComponent");
				if (const auto selection = findFurthestRappelComponent(startPosition); selection)
					localRappeller->rappel(selection, &startPosition, 4);
			}
		}
	}
	
	fclose(stdin);
	fclose(stdout);
	FreeConsole();
	FreeLibraryAndExitThread(moduleInstance, EXIT_SUCCESS);
}

bool DllMain(const HMODULE moduleInstance, const std::uint32_t callReason, void*)
{
	if (callReason == DLL_PROCESS_ATTACH)
	{
		if (const auto threadHandle = CreateThread(nullptr, 0, reinterpret_cast<PTHREAD_START_ROUTINE>(mainThread), moduleInstance, 0, nullptr);
			threadHandle != INVALID_HANDLE_VALUE)
			CloseHandle(threadHandle);
	}

	//
	// Remove our hooks.
	//
	else if (callReason == DLL_PROCESS_DETACH)
	{
		*reinterpret_cast<decltype(hooks::originalRappelComponentConstructor)*>(utilities::imageBase() + 0x702BA38) = hooks::originalRappelComponentConstructor;

		// Wait for any threads running our hooks to finish executing.
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return true;
}