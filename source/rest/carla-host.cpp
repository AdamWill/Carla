/*
 * Carla REST API Server
 * Copyright (C) 2018 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "common.hpp"

#include "CarlaHost.h"
#include "CarlaBackendUtils.hpp"

// -------------------------------------------------------------------------------------------------------------------

static bool gEngineRunning = false;

void engine_idle_handler()
{
    if (gEngineRunning)
        carla_engine_idle();
}

// -------------------------------------------------------------------------------------------------------------------

static void EngineCallback(void* ptr, EngineCallbackOpcode action, uint pluginId, int value1, int value2, float value3, const char* valueStr)
{
    carla_stdout("EngineCallback(%p, %u:%s, %u, %i, %i, %f, %s)",
                 ptr, (uint)action, EngineCallbackOpcode2Str(action), pluginId, value1, value2, value3, valueStr);

    char msgBuf[1024];
    std::snprintf(msgBuf, 1023, "Carla: %u %u %i %i %f %s\n", action, pluginId, value1, value2, value3, valueStr);
    msgBuf[1023] = '\0';

    switch (action)
    {
    case ENGINE_CALLBACK_ENGINE_STARTED:
        gEngineRunning = true;
        break;
    case ENGINE_CALLBACK_ENGINE_STOPPED:
    case ENGINE_CALLBACK_QUIT:
        gEngineRunning = false;
        break;
    default:
        break;
    }

    send_server_side_message(msgBuf);
}

static const char* FileCallback(void* ptr, FileCallbackOpcode action, bool isDir, const char* title, const char* filter)
{
    carla_stdout("FileCallback(%p, %u:%s, %s, %s, %s)",
                 ptr, (uint)action, FileCallbackOpcode(action), bool2str(isDir), title, filter);

    char msgBuf[1024];
    std::snprintf(msgBuf, 1023, "fc %u %i \"%s\" \"%s\"", action, isDir, title, filter);
    msgBuf[1023] = '\0';

    send_server_side_message(msgBuf);

    // FIXME, need to wait for response!
    return nullptr;
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_get_engine_driver_count(const std::shared_ptr<Session> session)
{
    const char* const buf = str_buf_uint(carla_get_engine_driver_count());
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_engine_driver_name(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int index = std::atoi(request->get_path_parameter("index").c_str());
    CARLA_SAFE_ASSERT_RETURN(index >= 0 /*&& index < INT_MAX*/,)

    const char* const buf = str_buf_string(carla_get_engine_driver_name(index));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_engine_driver_device_names(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int index = std::atoi(request->get_path_parameter("index").c_str());
    CARLA_SAFE_ASSERT_RETURN(index >= 0 /*&& index < INT_MAX*/,)

    const char* const buf = str_buf_string_array(carla_get_engine_driver_device_names(index));
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

void handle_carla_get_engine_driver_device_info(const std::shared_ptr<Session> session)
{
    const std::shared_ptr<const Request> request = session->get_request();

    const int index = std::atoi(request->get_path_parameter("index").c_str());
    CARLA_SAFE_ASSERT_RETURN(index >= 0 /*&& index < INT_MAX*/,)

    const std::string name = request->get_path_parameter("name");

    const EngineDriverDeviceInfo* const info = carla_get_engine_driver_device_info(index, name.c_str());

    char* jsonBuf;
    jsonBuf = json_buf_start();
    jsonBuf = json_buf_add_uint(jsonBuf, "hints", info->hints);
    jsonBuf = json_buf_add_uint_array(jsonBuf, "bufferSizes", info->bufferSizes);
    jsonBuf = json_buf_add_float_array(jsonBuf, "sampleRates", info->sampleRates);

    const char* const buf = json_buf_end(jsonBuf);
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

// -------------------------------------------------------------------------------------------------------------------

void handle_carla_engine_init(const std::shared_ptr<Session> session)
{
    // setup callbacks
    carla_set_engine_callback(EngineCallback, nullptr);
    carla_set_file_callback(FileCallback, nullptr);

    // handle request now
    const std::shared_ptr<const Request> request = session->get_request();

    const std::string driverName = request->get_path_parameter("driverName");
    const std::string clientName = request->get_path_parameter("clientName");

    const bool resp = carla_engine_init(driverName.c_str(), clientName.c_str());
    session->close(resp ? OK : BAD_REQUEST);
}

void handle_carla_engine_close(const std::shared_ptr<Session> session)
{
    const bool resp = carla_engine_close();
    session->close(resp ? OK : BAD_REQUEST);
}

void handle_carla_is_engine_running(const std::shared_ptr<Session> session)
{
    const bool resp = carla_is_engine_running();
    session->close(resp ? OK : BAD_REQUEST);
}

void handle_carla_set_engine_about_to_close(const std::shared_ptr<Session> session)
{
    const bool resp = carla_set_engine_about_to_close();
    session->close(resp ? OK : BAD_REQUEST);
}

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT void carla_set_engine_option(EngineOption option, int value, const char* valueStr);

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT bool carla_load_file(const char* filename);
CARLA_EXPORT bool carla_load_project(const char* filename);
CARLA_EXPORT bool carla_save_project(const char* filename);
CARLA_EXPORT bool carla_patchbay_connect(uint groupIdA, uint portIdA, uint groupIdB, uint portIdB);
CARLA_EXPORT bool carla_patchbay_disconnect(uint connectionId);
CARLA_EXPORT bool carla_patchbay_refresh(bool external);

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT void carla_transport_play();
CARLA_EXPORT void carla_transport_pause();
CARLA_EXPORT void carla_transport_bpm(double bpm);
CARLA_EXPORT void carla_transport_relocate(uint64_t frame);
CARLA_EXPORT uint64_t carla_get_current_transport_frame();
CARLA_EXPORT const CarlaTransportInfo* carla_get_transport_info();

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT uint32_t carla_get_current_plugin_count();
CARLA_EXPORT uint32_t carla_get_max_plugin_number();
CARLA_EXPORT bool carla_add_plugin(BinaryType btype, PluginType ptype,
                                   const char* filename, const char* name, const char* label, int64_t uniqueId,
                                   const void* extraPtr, uint options);
CARLA_EXPORT bool carla_remove_plugin(uint pluginId);
CARLA_EXPORT bool carla_remove_all_plugins();

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT const char* carla_rename_plugin(uint pluginId, const char* newName);
CARLA_EXPORT bool carla_clone_plugin(uint pluginId);
CARLA_EXPORT bool carla_replace_plugin(uint pluginId);
CARLA_EXPORT bool carla_switch_plugins(uint pluginIdA, uint pluginIdB);

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT bool carla_load_plugin_state(uint pluginId, const char* filename);
CARLA_EXPORT bool carla_save_plugin_state(uint pluginId, const char* filename);
CARLA_EXPORT bool carla_export_plugin_lv2(uint pluginId, const char* lv2path);

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT const CarlaPluginInfo* carla_get_plugin_info(uint pluginId);
CARLA_EXPORT const CarlaPortCountInfo* carla_get_audio_port_count_info(uint pluginId);
CARLA_EXPORT const CarlaPortCountInfo* carla_get_midi_port_count_info(uint pluginId);
CARLA_EXPORT const CarlaPortCountInfo* carla_get_parameter_count_info(uint pluginId);
CARLA_EXPORT const CarlaParameterInfo* carla_get_parameter_info(uint pluginId, uint32_t parameterId);
CARLA_EXPORT const CarlaScalePointInfo* carla_get_parameter_scalepoint_info(uint pluginId, uint32_t parameterId, uint32_t scalePointId);
CARLA_EXPORT const ParameterData* carla_get_parameter_data(uint pluginId, uint32_t parameterId);
CARLA_EXPORT const ParameterRanges* carla_get_parameter_ranges(uint pluginId, uint32_t parameterId);
CARLA_EXPORT const MidiProgramData* carla_get_midi_program_data(uint pluginId, uint32_t midiProgramId);
CARLA_EXPORT const CustomData* carla_get_custom_data(uint pluginId, uint32_t customDataId);
CARLA_EXPORT const char* carla_get_chunk_data(uint pluginId);

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT uint32_t carla_get_parameter_count(uint pluginId);
CARLA_EXPORT uint32_t carla_get_program_count(uint pluginId);
CARLA_EXPORT uint32_t carla_get_midi_program_count(uint pluginId);
CARLA_EXPORT uint32_t carla_get_custom_data_count(uint pluginId);
CARLA_EXPORT const char* carla_get_parameter_text(uint pluginId, uint32_t parameterId);
CARLA_EXPORT const char* carla_get_program_name(uint pluginId, uint32_t programId);
CARLA_EXPORT const char* carla_get_midi_program_name(uint pluginId, uint32_t midiProgramId);
CARLA_EXPORT const char* carla_get_real_plugin_name(uint pluginId);
CARLA_EXPORT int32_t carla_get_current_program_index(uint pluginId);
CARLA_EXPORT int32_t carla_get_current_midi_program_index(uint pluginId);
CARLA_EXPORT float carla_get_default_parameter_value(uint pluginId, uint32_t parameterId);
CARLA_EXPORT float carla_get_current_parameter_value(uint pluginId, uint32_t parameterId);
CARLA_EXPORT float carla_get_internal_parameter_value(uint pluginId, int32_t parameterId);
CARLA_EXPORT float carla_get_input_peak_value(uint pluginId, bool isLeft);
CARLA_EXPORT float carla_get_output_peak_value(uint pluginId, bool isLeft);

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT void carla_set_active(uint pluginId, bool onOff);
CARLA_EXPORT void carla_set_drywet(uint pluginId, float value);
CARLA_EXPORT void carla_set_volume(uint pluginId, float value);
CARLA_EXPORT void carla_set_balance_left(uint pluginId, float value);
CARLA_EXPORT void carla_set_balance_right(uint pluginId, float value);
CARLA_EXPORT void carla_set_panning(uint pluginId, float value);
CARLA_EXPORT void carla_set_ctrl_channel(uint pluginId, int8_t channel);
CARLA_EXPORT void carla_set_option(uint pluginId, uint option, bool yesNo);

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT void carla_set_parameter_value(uint pluginId, uint32_t parameterId, float value);
CARLA_EXPORT void carla_set_parameter_midi_channel(uint pluginId, uint32_t parameterId, uint8_t channel);
CARLA_EXPORT void carla_set_parameter_midi_cc(uint pluginId, uint32_t parameterId, int16_t cc);
CARLA_EXPORT void carla_set_program(uint pluginId, uint32_t programId);
CARLA_EXPORT void carla_set_midi_program(uint pluginId, uint32_t midiProgramId);
CARLA_EXPORT void carla_set_custom_data(uint pluginId, const char* type, const char* key, const char* value);
CARLA_EXPORT void carla_set_chunk_data(uint pluginId, const char* chunkData);

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT void carla_prepare_for_save(uint pluginId);
CARLA_EXPORT void carla_reset_parameters(uint pluginId);
CARLA_EXPORT void carla_randomize_parameters(uint pluginId);

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT void carla_send_midi_note(uint pluginId, uint8_t channel, uint8_t note, uint8_t velocity);

// -------------------------------------------------------------------------------------------------------------------

CARLA_EXPORT uint32_t carla_get_buffer_size();
CARLA_EXPORT double carla_get_sample_rate();

void handle_carla_get_last_error(const std::shared_ptr<Session> session)
{
    const char* const buf = carla_get_last_error();
    session->close(OK, buf, { { "Content-Length", size_buf(buf) } } );
}

CARLA_EXPORT const char* carla_get_host_osc_url_tcp();
CARLA_EXPORT const char* carla_get_host_osc_url_udp();

// -------------------------------------------------------------------------------------------------------------------
