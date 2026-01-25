<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
	import { moduleLoading, moduleError, setupWasmCallbacks } from '$lib/stores/game';

	let canvas: HTMLCanvasElement;
	let statusText = $state('Downloading...');
	let showOverlay = $state(true);

	onMount(() => {
		// Initialize C++ -> JS callbacks before loading WASM
		setupWasmCallbacks();
		// Set up the global Module object that Emscripten expects
		const Module = {
			print(...args: unknown[]) {
				console.log(...args);
			},
			canvas: canvas,
			setStatus(text: string) {
				if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
				if (text === Module.setStatus.last.text) return;
				const m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
				const now = Date.now();
				if (m && now - Module.setStatus.last.time < 30) return;
				Module.setStatus.last.time = now;
				Module.setStatus.last.text = text;
				if (m) {
					text = m[1];
				}
				statusText = text;
				if (!text) {
					showOverlay = false;
					moduleLoading.set(false);
				}
			},
			totalDependencies: 0,
			monitorRunDependencies(left: number) {
				this.totalDependencies = Math.max(this.totalDependencies, left);
				Module.setStatus(
					left
						? 'Preparing... (' + (this.totalDependencies - left) + '/' + this.totalDependencies + ')'
						: 'All downloads complete.'
				);
			}
		} as typeof Module & { setStatus: { last?: { time: number; text: string } } };

		Module.setStatus('Downloading...');

		// Expose Module globally for Emscripten
		(window as unknown as { Module: typeof Module }).Module = Module;

		// Handle WebGL context loss
		canvas.addEventListener('webglcontextlost', (e) => {
			alert('WebGL context lost. You will need to reload the page.');
			e.preventDefault();
		});

		// Prevent context menu on right-click
		canvas.addEventListener('contextmenu', (e) => e.preventDefault());

		// Prevent middle-click default behavior
		canvas.addEventListener('auxclick', (e) => e.preventDefault());

		// Load the game.js script
		const script = document.createElement('script');
		script.src = '/game.js';
		script.async = true;
		script.onerror = () => {
			moduleError.set('Failed to load game.js');
			moduleLoading.set(false);
			showOverlay = false;
		};
		document.body.appendChild(script);

		window.onerror = () => {
			Module.setStatus('Exception thrown, see JavaScript console');
			moduleError.set('Exception thrown, see JavaScript console');
		};
	});

</script>

<canvas bind:this={canvas} id="canvas" class="game-canvas" tabindex="-1"></canvas>

{#if showOverlay}
	<div class="overlay">
		<div class="spinner"></div>
		<div class="status">{statusText}</div>
	</div>
{/if}

<style>
	.game-canvas {
		position: absolute;
		top: 0;
		left: 0;
		width: 100%;
		height: 100%;
		display: block;
		border: none;
		background: #000;
	}

	.overlay {
		position: fixed;
		top: 0;
		left: 0;
		width: 100%;
		height: 100%;
		display: flex;
		flex-direction: column;
		justify-content: center;
		align-items: center;
		background-color: rgba(0, 0, 0, 0.8);
		z-index: 10;
		pointer-events: none;
	}

	.spinner {
		height: 50px;
		width: 50px;
		animation: rotation 0.8s linear infinite;
		border-left: 10px solid rgb(0, 150, 240);
		border-right: 10px solid rgb(0, 150, 240);
		border-bottom: 10px solid rgb(0, 150, 240);
		border-top: 10px solid rgb(100, 0, 200);
		border-radius: 100%;
		background-color: rgb(200, 100, 250);
	}

	@keyframes rotation {
		from {
			transform: rotate(0deg);
		}
		to {
			transform: rotate(360deg);
		}
	}

	.status {
		color: white;
		margin-top: 1em;
		font-family: sans-serif;
	}

</style>
