<script lang="ts">
	import { onMount } from 'svelte';
	import Canvas from '$lib/components/Canvas.svelte';
	import InfoPanel from '$lib/components/InfoPanel.svelte';
	import FilterControls from '$lib/components/FilterControls.svelte';
	import OverlayManager from '$lib/components/OverlayManager.svelte';
	import { initializeGame, moduleLoading, moduleError, selectObject } from '$lib/stores/game';

	onMount(() => {
		initializeGame();

		// Demo: select ISS after a short delay
		setTimeout(() => {
			selectObject(1);
		}, 500);
	});
</script>

<svelte:head>
	<title>Orbis - Space Object Tracker</title>
</svelte:head>

<main class="app">
	{#if $moduleLoading}
		<div class="loading">
			<div class="spinner"></div>
			<p>Loading Orbis...</p>
		</div>
	{:else if $moduleError}
		<div class="error">
			<h2>Failed to load</h2>
			<p>{$moduleError}</p>
		</div>
	{:else}
		<Canvas />
		<FilterControls />
		<InfoPanel />
		<OverlayManager />
	{/if}
</main>

<style>
	:global(html, body) {
		margin: 0;
		padding: 0;
		width: 100%;
		height: 100%;
		overflow: hidden;
		background: #000;
	}

	.app {
		width: 100vw;
		height: 100vh;
		position: relative;
	}

	.loading,
	.error {
		position: absolute;
		top: 50%;
		left: 50%;
		transform: translate(-50%, -50%);
		text-align: center;
		color: #fff;
		font-family: system-ui, -apple-system, sans-serif;
	}

	.spinner {
		width: 40px;
		height: 40px;
		margin: 0 auto 1rem;
		border: 3px solid rgba(255, 255, 255, 0.2);
		border-top-color: #4a90d9;
		border-radius: 50%;
		animation: spin 1s linear infinite;
	}

	@keyframes spin {
		to {
			transform: rotate(360deg);
		}
	}

	.error h2 {
		margin: 0 0 0.5rem 0;
		color: #ff6b6b;
	}

	.error p {
		margin: 0;
		color: rgba(255, 255, 255, 0.7);
	}
</style>
