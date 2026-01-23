<script lang="ts">
	import { overlays, toggleOverlay } from '$lib/stores/game';
	import type { OverlayState } from '$lib/wasm/types';

	interface OverlayOption {
		key: keyof OverlayState;
		label: string;
		icon: string;
	}

	const overlayOptions: OverlayOption[] = [
		{ key: 'grid', label: 'Grid', icon: '⊞' },
		{ key: 'atmosphere', label: 'Atmosphere', icon: '◐' },
		{ key: 'labels', label: 'Labels', icon: 'Aa' },
		{ key: 'orbits', label: 'Orbits', icon: '◯' },
		{ key: 'groundTracks', label: 'Ground Tracks', icon: '⌇' }
	];
</script>

<div class="overlay-controls">
	{#each overlayOptions as option (option.key)}
		<button
			class="overlay-btn"
			class:active={$overlays[option.key]}
			onclick={() => toggleOverlay(option.key)}
			title={option.label}
		>
			<span class="icon">{option.icon}</span>
			<span class="label">{option.label}</span>
		</button>
	{/each}
</div>

<style>
	.overlay-controls {
		position: fixed;
		bottom: 1rem;
		left: 50%;
		transform: translateX(-50%);
		display: flex;
		gap: 0.25rem;
		padding: 0.375rem;
		background: rgba(20, 25, 35, 0.95);
		border: 1px solid rgba(255, 255, 255, 0.1);
		border-radius: 8px;
		box-shadow: 0 4px 20px rgba(0, 0, 0, 0.5);
		backdrop-filter: blur(10px);
	}

	.overlay-btn {
		display: flex;
		align-items: center;
		gap: 0.5rem;
		padding: 0.5rem 0.75rem;
		background: transparent;
		border: none;
		border-radius: 6px;
		color: rgba(255, 255, 255, 0.6);
		font-family: system-ui, -apple-system, sans-serif;
		font-size: 13px;
		cursor: pointer;
		transition: all 0.2s;
	}

	.overlay-btn:hover {
		background: rgba(255, 255, 255, 0.1);
		color: rgba(255, 255, 255, 0.9);
	}

	.overlay-btn.active {
		background: rgba(74, 144, 217, 0.3);
		color: #fff;
	}

	.icon {
		font-size: 14px;
		opacity: 0.8;
	}

	.label {
		font-weight: 500;
	}

	@media (max-width: 640px) {
		.overlay-controls {
			flex-wrap: wrap;
			max-width: calc(100vw - 2rem);
			justify-content: center;
		}

		.label {
			display: none;
		}

		.overlay-btn {
			padding: 0.5rem;
		}
	}
</style>
