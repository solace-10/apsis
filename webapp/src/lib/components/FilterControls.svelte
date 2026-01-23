<script lang="ts">
	import { groups, toggleGroupVisibility, setGroupVisibility } from '$lib/stores/game';

	let expanded = true;

	function formatCount(count: number): string {
		if (count >= 1000) {
			return `${(count / 1000).toFixed(1)}k`;
		}
		return count.toString();
	}

	function handleToggleAll(visible: boolean) {
		for (const group of $groups) {
			setGroupVisibility(group.id, visible);
		}
	}
</script>

<div class="filter-panel" class:collapsed={!expanded}>
	<header class="panel-header">
		<button class="toggle-btn" onclick={() => (expanded = !expanded)} aria-label="Toggle panel">
			<span class="toggle-icon">{expanded ? '◀' : '▶'}</span>
		</button>
		{#if expanded}
			<h2>Object Groups</h2>
			<div class="header-actions">
				<button class="action-btn" onclick={() => handleToggleAll(true)} title="Show all">
					All
				</button>
				<button class="action-btn" onclick={() => handleToggleAll(false)} title="Hide all">
					None
				</button>
			</div>
		{/if}
	</header>

	{#if expanded}
		<div class="panel-content">
			{#each $groups as group (group.id)}
				<label class="group-item">
					<input
						type="checkbox"
						checked={group.visible}
						onchange={() => toggleGroupVisibility(group.id)}
					/>
					<span class="group-color" style="--color: {group.color}"></span>
					<span class="group-name">{group.name}</span>
					<span class="group-count">{formatCount(group.count)}</span>
				</label>
			{/each}
		</div>
	{/if}
</div>

<style>
	.filter-panel {
		position: fixed;
		top: 1rem;
		left: 1rem;
		width: 260px;
		background: rgba(20, 25, 35, 0.95);
		border: 1px solid rgba(255, 255, 255, 0.1);
		border-radius: 8px;
		color: #fff;
		font-family: system-ui, -apple-system, sans-serif;
		font-size: 14px;
		box-shadow: 0 4px 20px rgba(0, 0, 0, 0.5);
		backdrop-filter: blur(10px);
		transition: width 0.2s ease;
	}

	.filter-panel.collapsed {
		width: auto;
	}

	.panel-header {
		display: flex;
		align-items: center;
		gap: 0.75rem;
		padding: 0.75rem 1rem;
		border-bottom: 1px solid rgba(255, 255, 255, 0.1);
	}

	.collapsed .panel-header {
		border-bottom: none;
		padding: 0.75rem;
	}

	.panel-header h2 {
		margin: 0;
		font-size: 14px;
		font-weight: 600;
		flex: 1;
	}

	.toggle-btn {
		background: none;
		border: none;
		color: rgba(255, 255, 255, 0.6);
		cursor: pointer;
		padding: 0.25rem;
		display: flex;
		align-items: center;
		justify-content: center;
		transition: color 0.2s;
	}

	.toggle-btn:hover {
		color: #fff;
	}

	.toggle-icon {
		font-size: 10px;
	}

	.header-actions {
		display: flex;
		gap: 0.5rem;
	}

	.action-btn {
		background: rgba(255, 255, 255, 0.1);
		border: none;
		border-radius: 4px;
		color: rgba(255, 255, 255, 0.7);
		font-size: 11px;
		padding: 0.25rem 0.5rem;
		cursor: pointer;
		transition: all 0.2s;
	}

	.action-btn:hover {
		background: rgba(255, 255, 255, 0.2);
		color: #fff;
	}

	.panel-content {
		padding: 0.5rem;
		max-height: calc(100vh - 8rem);
		overflow-y: auto;
	}

	.group-item {
		display: flex;
		align-items: center;
		gap: 0.75rem;
		padding: 0.5rem;
		border-radius: 4px;
		cursor: pointer;
		transition: background 0.2s;
	}

	.group-item:hover {
		background: rgba(255, 255, 255, 0.05);
	}

	.group-item input[type='checkbox'] {
		width: 16px;
		height: 16px;
		accent-color: #4a90d9;
		cursor: pointer;
	}

	.group-color {
		width: 12px;
		height: 12px;
		border-radius: 3px;
		background: var(--color);
		flex-shrink: 0;
	}

	.group-name {
		flex: 1;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}

	.group-count {
		color: rgba(255, 255, 255, 0.5);
		font-size: 12px;
		font-variant-numeric: tabular-nums;
	}
</style>
