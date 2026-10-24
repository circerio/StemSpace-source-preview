#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace audio_plugin::qualified_model {

inline constexpr std::string_view kContractId =
    "stemgenrt58-teacher004-ema-step45750-int8-pdc256";
inline constexpr std::string_view kModelSha256 =
    "77164d6a581fafb2a31f53fd8ffde44c07cf618472952a4cdba14e68dda3b8b9";
inline constexpr std::string_view kCheckpointSha256 =
    "7fcd444f83985c0aab0c76923c4355fea3c3e11aa32c81410a6bb9388b755154";
inline constexpr std::string_view kModelStateSha256 =
    "f78c49b3755d6a71b7890482d3a40a5662b417b7a63ed3ad9d393cfb0da0037b";
inline constexpr std::string_view kOutputAlignment =
    "previous_input_chunk";
inline constexpr std::string_view kPreroll =
    "discard_first_output_hop_after_reset";
inline constexpr std::int64_t kModelByteSize =
    37529132;
inline constexpr int kSampleRate = 44100;
inline constexpr int kNumChannels = 2;
inline constexpr int kNumStems = 4;
inline constexpr int kHopSamples = 128;
inline constexpr int kAnalysisWindowSamples = 1024;
inline constexpr int kAnalysisHistorySamples = 896;
inline constexpr int kSynthesisFrameSamples = 256;
inline constexpr int kFusionHiddenLayers = 2;
inline constexpr int kFusionHiddenSize = 1000;
inline constexpr int kModelOutputDelayChunks = 1;
inline constexpr int kAsyncQueueDelayChunks = 1;
inline constexpr int kPluginLatencyChunks = 2;
inline constexpr int kPluginLatencySamples = 256;
inline constexpr int kDrumsSourceIndex = 0;
inline constexpr int kBassSourceIndex = 1;
inline constexpr int kVocalsSourceIndex = 2;
inline constexpr int kOtherSourceIndex = 3;
inline constexpr int kResidualSourceIndex = 3;

inline constexpr auto kSourceOrder = std::to_array<int>({
    kDrumsSourceIndex, kBassSourceIndex, kVocalsSourceIndex, kOtherSourceIndex,
});

inline constexpr auto kInputNames = std::to_array<std::string_view>({
    "audio_chunk",
    "audio_history",
    "fusion_hidden",
    "spectral_numerator_tail",
    "waveform_tail",
    "attention_keys",
    "attention_values",
    "spec_memory_hidden",
    "waveform_memory_hidden",
});
inline constexpr auto kInputAudioShape = std::to_array<std::int64_t>(
    {1, 2, 128});
inline constexpr auto kInputHistoryShape = std::to_array<std::int64_t>(
    {1, 2, 896});
inline constexpr auto kInputHiddenShape = std::to_array<std::int64_t>(
    {2, 1, 1000});
inline constexpr auto kInputSpectralTailShape = std::to_array<std::int64_t>(
    {1, 4, 2, 128});
inline constexpr auto kInputWaveformTailShape = std::to_array<std::int64_t>(
    {1, 4, 2, 128});
inline constexpr auto kInputAttentionKeysShape = std::to_array<std::int64_t>(
    {1, 31, 64});
inline constexpr auto kInputAttentionValuesShape = std::to_array<std::int64_t>(
    {1, 31, 128});
inline constexpr auto kInputSpecMemoryHiddenShape = std::to_array<std::int64_t>(
    {1, 1, 500});
inline constexpr auto kInputWaveformMemoryHiddenShape = std::to_array<std::int64_t>(
    {1, 1, 500});

inline constexpr auto kOutputNames = std::to_array<std::string_view>({
    "separated_chunk",
    "next_audio_history",
    "next_fusion_hidden",
    "next_spectral_numerator_tail",
    "next_waveform_tail",
    "next_attention_keys",
    "next_attention_values",
    "next_spec_memory_hidden",
    "next_waveform_memory_hidden",
});
inline constexpr auto kOutputSeparatedShape = std::to_array<std::int64_t>(
    {1, 4, 2, 128});
inline constexpr auto kOutputHistoryShape = std::to_array<std::int64_t>(
    {1, 2, 896});
inline constexpr auto kOutputHiddenShape = std::to_array<std::int64_t>(
    {2, 1, 1000});
inline constexpr auto kOutputSpectralTailShape = std::to_array<std::int64_t>(
    {1, 4, 2, 128});
inline constexpr auto kOutputWaveformTailShape = std::to_array<std::int64_t>(
    {1, 4, 2, 128});
inline constexpr auto kOutputAttentionKeysShape = std::to_array<std::int64_t>(
    {1, 31, 64});
inline constexpr auto kOutputAttentionValuesShape = std::to_array<std::int64_t>(
    {1, 31, 128});
inline constexpr auto kOutputSpecMemoryHiddenShape = std::to_array<std::int64_t>(
    {1, 1, 500});
inline constexpr auto kOutputWaveformMemoryHiddenShape = std::to_array<std::int64_t>(
    {1, 1, 500});

struct MetadataEntry {
  std::string_view key;
  std::string_view value;
};

inline constexpr std::size_t kAttentionKeyElements =
    kInputAttentionKeysShape[0] * kInputAttentionKeysShape[1] * kInputAttentionKeysShape[2];
inline constexpr std::size_t kAttentionValueElements =
    kInputAttentionValuesShape[0] * kInputAttentionValuesShape[1] * kInputAttentionValuesShape[2];

inline constexpr std::size_t kSpecMemoryHiddenElements =
    kInputSpecMemoryHiddenShape[0] * kInputSpecMemoryHiddenShape[1] * kInputSpecMemoryHiddenShape[2];
inline constexpr std::size_t kWaveformMemoryHiddenElements =
    kInputWaveformMemoryHiddenShape[0] * kInputWaveformMemoryHiddenShape[1] * kInputWaveformMemoryHiddenShape[2];

inline constexpr auto kMetadata = std::to_array<MetadataEntry>({
    {"stemgenrt.runtime_variant",
     R"meta(branch-memory-seventeen-fused-qkv-u8s8-reduced-precise-v1)meta"},
    {"stemgenrt.state_family",
     R"meta(latency58-attention-private-branch-gru500-zero-projections-v1)meta"},
    {"stemgenrt.architecture",
     R"meta({"additional_audio_buffering_samples": 0, "additional_fft_transforms": 0, "additional_neural_parameters": 4952632, "additional_state_elements_per_stream": 6952, "additional_state_tensors": 4, "branch_memory_added_state_elements_per_stream": 1000, "branch_memory_added_state_tensors": 2, "branch_memory_channels": 500, "branch_memory_input": "normalized refined branch features before attention", "branch_memory_layers": 1, "branch_memory_output": "zero-initialized linear residual after branch normalization", "branch_memory_parameters": 3506000, "carrier_forward_fft_reused_from_features": true, "carrier_n_fft": 1024, "component_semantics": "spectral and waveform are before discrepancy correction", "encoder_initialization": "FP64 effective-kernel window transfer, rounded once to FP32", "encoder_transfer_bf16_equivalence_claimed": false, "extra_audio_buffering_samples": 0, "extra_stream_state_tensors": 0, "feature_history_samples": 896, "feature_n_fft": 1024, "feature_window": "Wang2021 K1024 M128 d0 asymmetric analysis", "fixed_residual_share": 0.0625, "flush_hops": 1, "fusion_refinement": "fused + expand(SiLU(reduce(fused))) before branch normalization", "fusion_refinement_input_features": 1000, "fusion_refinement_parameters": 256000, "fusion_refinement_rank": 128, "future_callbacks_beyond_received_input": 0, "graph_alignment_samples": 128, "hop_samples": 128, "host_queue_implemented_in_this_module": false, "host_queue_samples": 128, "intended_total_latency_samples": 256, "magnitude_features": "log1p(sqrt(real^2+imag^2+1e-12)/sqrt(max(frame_mean_power,1e-8)))", "magnitude_normalization_axes": "current-frame stereo and frequency only", "native_host_qualified": false, "output_policy_version": "latency58-c204-fixed-residual-sixteenth-v1", "phase_correction": "i*carrier times source-centered real coefficients at interior bins", "phase_correction_coordinate": "before inherited source calibration and discrepancy correction", "phase_endpoint_coefficients": "exact zero at DC and Nyquist", "phase_head_parameters": 293632, "phase_head_rank": 64, "precision": "float32", "public_fusion_state_scale": 3.814697265625e-06, "raw_output_semantics": "native DBV plus fixed discrepancy correction; native Other", "recurrent_cadence_changed_from_hann_hop128_parent": false, "sample_rate": 44100, "samplewise_latest_input_minus_output": "255-p for output callback sample p=0..127", "source_order": ["drums", "bass", "vocals", "other"], "spectral_denominator_rule": "sum of the two analysis-synthesis overlap products", "spectral_mask_bins": 513, "spectral_output_crop": [768, 1024], "spectral_synthesis_window": "matched pair with Hann256 analysis-synthesis product", "state_family": "latency58-attention-private-branch-gru500-zero-projections-v1", "state_names": ["audio_history", "fusion_hidden", "spectral_numerator_tail", "waveform_tail", "attention_keys", "attention_values", "spec_memory_hidden", "waveform_memory_hidden"], "synthesis_frame_samples": 256, "temporal_attention": "refined fused features plus causal softmax(Q K.T / sqrt(64)) V output", "temporal_attention_history_samples": 3968, "temporal_attention_key_channels": 64, "temporal_attention_parameters": 384000, "temporal_attention_value_channels": 128, "temporal_attention_window_frames": 32, "version": "latency58-attention-private-branch-gru500-zero-projections-v1", "waveform_synthesis_window": "unchanged periodic Hann256"})meta"},
    {"stemgenrt.model_state_sha256",
     R"meta(f78c49b3755d6a71b7890482d3a40a5662b417b7a63ed3ad9d393cfb0da0037b)meta"},
    {"stemgenrt.sample_rate",
     R"meta(44100)meta"},
    {"stemgenrt.hop_samples",
     R"meta(128)meta"},
    {"stemgenrt.analysis_fft_samples",
     R"meta(1024)meta"},
    {"stemgenrt.synthesis_frame_samples",
     R"meta(256)meta"},
    {"stemgenrt.graph_output_delay_samples",
     R"meta(128)meta"},
    {"stemgenrt.source_order",
     R"meta(drums,bass,vocals,other)meta"},
    {"stemgenrt.external_data",
     R"meta(false)meta"},
    {"stemgenrt.state_names",
     R"meta(["audio_history", "fusion_hidden", "spectral_numerator_tail", "waveform_tail", "attention_keys", "attention_values", "spec_memory_hidden", "waveform_memory_hidden"])meta"},
    {"stemgenrt.state_shapes",
     R"meta([[1, 2, 896], [2, 1, 1000], [1, 4, 2, 128], [1, 4, 2, 128], [1, 31, 64], [1, 31, 128], [1, 1, 500], [1, 1, 500]])meta"},
    {"stemgenrt.public_fusion_state_scale",
     R"meta(3.814697265625e-06)meta"},
    {"stemgenrt.public_branch_memory_state_scale",
     R"meta(3.814697265625e-06)meta"},
    {"stemgenrt.initial_state",
     R"meta(all_zeros)meta"},
    {"stemgenrt.preroll",
     R"meta(discard_first_output_hop_after_reset)meta"},
    {"stemgenrt.flush_hops",
     R"meta(1)meta"},
    {"stemgenrt.native_host_qualified",
     R"meta(false)meta"},
    {"stemgenrt.quality_measured",
     R"meta(false)meta"},
    {"stemgenrt.output_policy",
     R"meta(DBV += (delayed mixture - sum(native raw4))/16; Other = delayed mixture - sum(corrected DBV))meta"},
    {"stemgenrt.fixed_residual_share",
     R"meta(0.0625)meta"},
    {"stemgenrt.checkpoint_sha256",
     R"meta(7fcd444f83985c0aab0c76923c4355fea3c3e11aa32c81410a6bb9388b755154)meta"},
    {"stemgenrt.integer_weight_precision",
     R"meta(Seventeen S8 symmetric reduced-range matrices [-64,64])meta"},
    {"stemgenrt.activation_quantization",
     R"meta(Dynamic U8 per one-frame projection; FP32 scales and dequantization)meta"},
    {"stemgenrt.inference_precision",
     R"meta(FP64 quantizer ancestors; integer projections and output decoding FP32; public states FP32)meta"},
    {"stemgenrt.floating_additional_layers",
     R"meta(phase factors, fusion refinement, temporal output, GRU biases/nonlinearities)meta"},
    {"stemgenrt.refinement_silu_implementation",
     R"meta(x / (1 + exp(-x)); FP64 quantizer ancestor)meta"},
    {"stemgenrt.integer_precision_transform_sha256",
     R"meta(f53ea1c2023f9ff304e4d5be9bcb57e6faed715947196ea9a719af79b2215dbb)meta"},
    {"stemgenrt.embedded_quality_scope",
     R"meta(Source FP32 checkpoint only; deployment graph quality is recorded separately)meta"},
    {"parent.stemgenrt.precise_node_count",
     R"meta(396)meta"},
    {"parent.stemgenrt.integer_exporter_sha256",
     R"meta(189ca340d41422ca042131fbeb44d8c5a90968423bc4e7227ac22ded0a2075c9)meta"},
    {"stemgenrt.parent_graph_sha256",
     R"meta(91bdbc9319e6513d2320d9914de8d0ff2dacafeb2459ea74229b9b6d332fb359)meta"},
    {"stemgenrt.integer_exporter_sha256",
     R"meta(4a563c2317f245b76e3135059a6f491aafa95b6bff3d13e2beeb9e9198e6f589)meta"},
    {"stemgenrt.additional_branch_integer_projections",
     R"meta(4)meta"},
    {"stemgenrt.deployment_quality_status",
     R"meta(unmeasured; parent score does not score this graph)meta"},
    {"stemgenrt.parent_fourteen_exporter_sha256",
     R"meta(c547aa9375a58918fd1cc80483ef6c1daaa145716d860ad8d62b61bf9eda0820)meta"},
    {"stemgenrt.additional_branch_output_integer_projections",
     R"meta(2)meta"},
    {"stemgenrt.attention_fusion_exporter_sha256",
     R"meta(2b63eeb90d32d378155f73b346258e5ea50eb5c50f280a17d000d53dd351f62b)meta"},
    {"stemgenrt.attention_projection_fusion",
     R"meta(QKV columns concatenated; FP64 product and original query last-frame slice)meta"},
    {"stemgenrt.parent_sixteen_exporter_sha256",
     R"meta(dbb298b58738b98471d4a5e25c31288fa07bb22e211d9973f73555ccbabfdeb5)meta"},
    {"stemgenrt.additional_attention_integer_products",
     R"meta(One fused QKV product; three column groups)meta"},
    {"stemgenrt.model_name",
     R"meta(StemgenRT-5.8)meta"},
    {"stemgenrt.checkpoint_weight_role",
     R"meta(averaged_inference)meta"},
    {"stemgenrt.training_plan_sha256",
     R"meta(2ef8b5720e7a232936b404106a4d526421090aec0262e317f0ed26802a4c36cd)meta"},
    {"stemgenrt.snapshot_step",
     R"meta(2000)meta"},
    {"stemgenrt.training_updates",
     R"meta(45750)meta"},
    {"stemgenrt.training_precision",
     R"meta(bf16 learned operations; FP32 synthesis and public states)meta"},
    {"stemgenrt.training_teacher_used",
     R"meta(true)meta"},
    {"stemgenrt.teacher_used_in_inference",
     R"meta(false)meta"},
    {"stemgenrt.teacher_coefficient",
     R"meta(1.0)meta"},
    {"stemgenrt.training_objective",
     R"meta(prospective-ordinary-primary-sdr-two-fifths-v1)meta"},
    {"stemgenrt.ordinary_primary_sdr_weight",
     R"meta(0.4)meta"},
    {"stemgenrt.training_tracks",
     R"meta(301)meta"},
    {"stemgenrt.source_checkpoint_full14_sdr_db",
     R"meta(4.564401858994641)meta"},
    {"stemgenrt.source_checkpoint_full14_result_sha256",
     R"meta(fc1319e23b36524d70a526bb46f1afa8b9bf0285a917d431d5e9d4723166b52c)meta"},
    {"stemgenrt.intended_external_host_queue_samples",
     R"meta(128)meta"},
    {"stemgenrt.intended_total_latency_samples",
     R"meta(256)meta"},
    {"stemgenrt.additional_audio_buffering_samples",
     R"meta(0)meta"},
    {"stemgenrt.exporter_sha256",
     R"meta(19729cb5e8ed185fa6424cb75814bde025f652172b97aeeb3863fcc5db511cf1)meta"},
});

static_assert(kInputAudioShape == std::array<std::int64_t, 3>{1, 2, 128});
static_assert(kInputHistoryShape == std::array<std::int64_t, 3>{1, 2, 896});
static_assert(kInputHiddenShape == std::array<std::int64_t, 3>{2, 1, 1000});
static_assert(kInputSpectralTailShape == std::array<std::int64_t, 4>{1, 4, 2, 128});
static_assert(kInputWaveformTailShape == kInputSpectralTailShape);
static_assert(kOutputSeparatedShape == kInputSpectralTailShape);
static_assert(kOutputHistoryShape == kInputHistoryShape);
static_assert(kOutputHiddenShape == kInputHiddenShape);
static_assert(kOutputSpectralTailShape == kInputSpectralTailShape);
static_assert(kOutputWaveformTailShape == kInputWaveformTailShape);
static_assert(kAnalysisWindowSamples == kAnalysisHistorySamples + kHopSamples);
static_assert(kSynthesisFrameSamples == 2 * kHopSamples);
static_assert(kInputNames.size() == 9 && kOutputNames.size() == 9);
static_assert(kInputSpecMemoryHiddenShape == std::array<std::int64_t, 3>{1, 1, 500});
static_assert(kInputWaveformMemoryHiddenShape == kInputSpecMemoryHiddenShape);
static_assert(kOutputSpecMemoryHiddenShape == kInputSpecMemoryHiddenShape);
static_assert(kOutputWaveformMemoryHiddenShape == kInputWaveformMemoryHiddenShape);
static_assert(kInputAttentionKeysShape == std::array<std::int64_t, 3>{1, 31, 64});
static_assert(kInputAttentionValuesShape == std::array<std::int64_t, 3>{1, 31, 128});
static_assert(kOutputAttentionKeysShape == kInputAttentionKeysShape);
static_assert(kOutputAttentionValuesShape == kInputAttentionValuesShape);
static_assert(kNumChannels == 2 && kNumStems == 4);
static_assert(kSampleRate == 44100 && kHopSamples == 128);
static_assert(kModelOutputDelayChunks == 1 && kAsyncQueueDelayChunks == 1);
static_assert(kPluginLatencyChunks == kModelOutputDelayChunks + kAsyncQueueDelayChunks);
static_assert(kPluginLatencySamples == kPluginLatencyChunks * kHopSamples);
static_assert(kPluginLatencySamples == 256);
static_assert(kSourceOrder == std::array<int, 4>{0, 1, 2, 3});
static_assert(kOtherSourceIndex == kResidualSourceIndex);
static_assert(kOutputAlignment == "previous_input_chunk");
static_assert(kPreroll == "discard_first_output_hop_after_reset");

}  // namespace audio_plugin::qualified_model
