<script lang="ts">
	import { onMount } from 'svelte';
	import '../app.css';
	import Canvas from '$lib/components/Canvas.svelte';
	import InfoPanel from '$lib/components/InfoPanel.svelte';
	import FilterControls from '$lib/components/FilterControls.svelte';
	// import OverlayManager from '$lib/components/OverlayManager.svelte';
	import { initializeGame } from '$lib/stores/game';

	onMount(() => {
		// Initialize mock stores (will be replaced by real WASM bindings later)
		initializeGame();
	});
</script>

<svelte:head>
	<title>Orbis - Space Object Tracker</title>
</svelte:head>

<main class="app">
	<Canvas />
	<FilterControls />
	<InfoPanel />
	<!-- <OverlayManager /> -->
</main>

<style>
	:global(html, body) {
		margin: 0;
		padding: 0;
		width: 100%;
		height: 100%;
		overflow: hidden;
		background: var(--bg-deep);
	}

	.app {
		width: 100vw;
		height: 100vh;
		position: relative;
	}
</style>
