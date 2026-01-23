<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
	import { getModule } from '$lib/wasm/module';

	let canvas: HTMLCanvasElement;
	let resizeObserver: ResizeObserver | null = null;

	onMount(() => {
		const module = getModule();
		module.initCanvas(canvas);

		// Handle resize
		resizeObserver = new ResizeObserver((entries) => {
			for (const entry of entries) {
				const { width, height } = entry.contentRect;
				canvas.width = width * window.devicePixelRatio;
				canvas.height = height * window.devicePixelRatio;
				module.resize(canvas.width, canvas.height);
			}
		});

		resizeObserver.observe(canvas);
	});

	onDestroy(() => {
		resizeObserver?.disconnect();
	});
</script>

<canvas bind:this={canvas} class="game-canvas"></canvas>

<style>
	.game-canvas {
		position: absolute;
		top: 0;
		left: 0;
		width: 100%;
		height: 100%;
		display: block;
		background: #000;
	}
</style>
