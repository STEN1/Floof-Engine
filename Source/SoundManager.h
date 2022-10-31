#pragma once
#include "al.h"
#include "alc.h"

#include <cstring>
#include <iostream>
#include <fstream>
#include <type_traits>
#include <vector>
#include <bit>

namespace FLOOF {

	class SoundManager {
		public:
			SoundManager();
		private:
			bool check_al_errors(const std::string& filename, const std::uint_fast32_t line);
			bool check_alc_errors(const std::string& filename, const std::uint_fast32_t line, ALCdevice* device);

		template<typename alFunction, typename... Params>
		auto alCallImpl(const char* filename,
			const std::uint_fast32_t line,
			alFunction function,
			Params... params)
			->typename std::enable_if_t<!std::is_same_v<void, decltype(function(params...))>, decltype(function(params...))>
		{
			auto ret = function(std::forward<Params>(params)...);
			check_al_errors(filename, line);
			return ret;
		}

		template<typename alFunction, typename... Params>
		auto alCallImpl(const char* filename,
			const std::uint_fast32_t line,
			alFunction function,
			Params... params)
			->typename std::enable_if_t<std::is_same_v<void, decltype(function(params...))>, bool>
		{
			function(std::forward<Params>(params)...);
			return check_al_errors(filename, line);
		}

		template<typename alcFunction, typename... Params>
		auto alcCallImpl(const char* filename,
			const std::uint_fast32_t line,
			alcFunction function,
			ALCdevice* device,
			Params... params)
			->typename std::enable_if_t<std::is_same_v<void, decltype(function(params...))>, bool>
		{
			function(std::forward<Params>(params)...);
			return check_alc_errors(filename, line, device);
		}

		template<typename alcFunction, typename ReturnType, typename... Params>
		auto alcCallImpl(const char* filename,
			const std::uint_fast32_t line,
			alcFunction function,
			ReturnType& returnValue,
			ALCdevice* device,
			Params... params)
			->typename std::enable_if_t<!std::is_same_v<void, decltype(function(params...))>, bool>
		{
			returnValue = function(std::forward<Params>(params)...);
			return check_alc_errors(filename, line, device);
		}


		bool get_available_devices(std::vector<std::string>& devicesVec, ALCdevice* device);


		std::int32_t convert_to_int(char* buffer, std::size_t len);

		bool load_wav_file_header(std::ifstream& file,
		                          std::uint8_t& channels,
		                          std::int32_t& sampleRate,
		                          std::uint8_t& bitsPerSample,
		                          ALsizei& size);

		char* load_wav(const std::string& filename,
			std::uint8_t& channels,
			std::int32_t& sampleRate,
			std::uint8_t& bitsPerSample,
			ALsizei& size);

	};




}
