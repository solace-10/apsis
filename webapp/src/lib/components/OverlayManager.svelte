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
		background: var(--bg-panel);
		border: 1px solid var(--border-subtle);
		border-radius: 4px;
	}

	.overlay-btn {
		display: flex;
		align-items: center;
		gap: 0.5rem;
		padding: 0.5rem 0.75rem;
		background: transparent;
		border: 1px solid transparent;
		border-radius: 3px;
		color: var(--text-secondary);
		font-family: var(--font-mono);
		font-size: 0.75rem;
		text-transform: uppercase;
		letter-spacing: 0.03em;
		cursor: pointer;
		transition: all 0.15s ease;
	}

	.overlay-btn:hover {
		background: var(--bg-hover);
		color: var(--text-primary);
	}

	.overlay-btn.active {
		background: var(--accent-primary-dim);
		border-color: var(--accent-primary);
		color: var(--accent-primary);
	}

	.icon {
		font-size: 14px;
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
