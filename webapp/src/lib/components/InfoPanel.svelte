<script lang="ts">
	import { selectedObject, selectObject } from '$lib/stores/game';

	function formatNumber(value: number, decimals: number = 2): string {
		return value.toFixed(decimals);
	}

	function formatCoordinate(value: number, isLatitude: boolean): string {
		const abs = Math.abs(value);
		const suffix = isLatitude ? (value >= 0 ? 'N' : 'S') : value >= 0 ? 'E' : 'W';
		return `${formatNumber(abs, 4)}° ${suffix}`;
	}

	function getObjectTypeLabel(type: string): string {
		const labels: Record<string, string> = {
			satellite: 'Satellite',
			debris: 'Debris',
			rocket_body: 'Rocket Body',
			space_station: 'Space Station',
			unknown: 'Unknown'
		};
		return labels[type] || type;
	}

	function handleClose() {
		selectObject(null);
	}
</script>

{#if $selectedObject}
	<div class="info-panel">
		<header class="panel-header">
			<h2>{$selectedObject.name}</h2>
			<button class="close-btn" onclick={handleClose} aria-label="Close panel">×</button>
		</header>

		<div class="panel-content">
			<section class="info-section">
				<h3>Identification</h3>
				<dl>
					<dt>NORAD ID</dt>
					<dd>{$selectedObject.noradId}</dd>
					<dt>Type</dt>
					<dd>{getObjectTypeLabel($selectedObject.objectType)}</dd>
					<dt>Group</dt>
					<dd>
						<span class="group-badge" style="--group-color: {$selectedObject.group.color}">
							{$selectedObject.group.name}
						</span>
					</dd>
				</dl>
			</section>

			<section class="info-section">
				<h3>Current Position</h3>
				<dl>
					<dt>Altitude</dt>
					<dd>{formatNumber($selectedObject.altitude, 1)} km</dd>
					<dt>Velocity</dt>
					<dd>{formatNumber($selectedObject.velocity, 2)} km/s</dd>
					<dt>Latitude</dt>
					<dd>{formatCoordinate($selectedObject.latitude, true)}</dd>
					<dt>Longitude</dt>
					<dd>{formatCoordinate($selectedObject.longitude, false)}</dd>
				</dl>
			</section>

			<section class="info-section">
				<h3>Orbital Elements</h3>
				<dl>
					<dt>Semi-major Axis</dt>
					<dd>{formatNumber($selectedObject.semiMajorAxis, 1)} km</dd>
					<dt>Eccentricity</dt>
					<dd>{formatNumber($selectedObject.eccentricity, 6)}</dd>
					<dt>Inclination</dt>
					<dd>{formatNumber($selectedObject.inclination, 2)}°</dd>
					<dt>RAAN</dt>
					<dd>{formatNumber($selectedObject.raan, 2)}°</dd>
					<dt>Arg. of Perigee</dt>
					<dd>{formatNumber($selectedObject.argOfPerigee, 2)}°</dd>
					<dt>Mean Anomaly</dt>
					<dd>{formatNumber($selectedObject.meanAnomaly, 2)}°</dd>
				</dl>
			</section>
		</div>
	</div>
{/if}

<style>
	.info-panel {
		position: fixed;
		top: 1rem;
		right: 1rem;
		width: 320px;
		max-height: calc(100vh - 2rem);
		overflow-y: auto;
		background: rgba(20, 25, 35, 0.95);
		border: 1px solid rgba(255, 255, 255, 0.1);
		border-radius: 8px;
		color: #fff;
		font-family: system-ui, -apple-system, sans-serif;
		font-size: 14px;
		box-shadow: 0 4px 20px rgba(0, 0, 0, 0.5);
		backdrop-filter: blur(10px);
	}

	.panel-header {
		display: flex;
		align-items: center;
		justify-content: space-between;
		padding: 1rem;
		border-bottom: 1px solid rgba(255, 255, 255, 0.1);
	}

	.panel-header h2 {
		margin: 0;
		font-size: 16px;
		font-weight: 600;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}

	.close-btn {
		background: none;
		border: none;
		color: rgba(255, 255, 255, 0.6);
		font-size: 24px;
		cursor: pointer;
		padding: 0;
		line-height: 1;
		transition: color 0.2s;
	}

	.close-btn:hover {
		color: #fff;
	}

	.panel-content {
		padding: 0.5rem;
	}

	.info-section {
		padding: 0.75rem;
		border-bottom: 1px solid rgba(255, 255, 255, 0.05);
	}

	.info-section:last-child {
		border-bottom: none;
	}

	.info-section h3 {
		margin: 0 0 0.75rem 0;
		font-size: 11px;
		font-weight: 600;
		text-transform: uppercase;
		letter-spacing: 0.05em;
		color: rgba(255, 255, 255, 0.5);
	}

	dl {
		display: grid;
		grid-template-columns: auto 1fr;
		gap: 0.5rem 1rem;
		margin: 0;
	}

	dt {
		color: rgba(255, 255, 255, 0.6);
	}

	dd {
		margin: 0;
		text-align: right;
		font-variant-numeric: tabular-nums;
	}

	.group-badge {
		display: inline-block;
		padding: 0.125rem 0.5rem;
		background: var(--group-color);
		border-radius: 4px;
		font-size: 12px;
		font-weight: 500;
		color: #000;
	}
</style>
