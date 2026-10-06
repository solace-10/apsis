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

{#if $groups.length > 0}
	<div class="filter-panel panel" class:collapsed={!expanded}>
		<header class="panel-header">
			<button
				class="icon-btn toggle-btn"
				onclick={() => (expanded = !expanded)}
				aria-label={expanded ? 'Collapse object groups' : 'Expand object groups'}
				aria-expanded={expanded}
			>
				<svg class="icon" viewBox="0 0 16 16" aria-hidden="true">
					<path d={expanded ? 'M10 3L5 8l5 5' : 'M6 3l5 5-5 5'} />
				</svg>
			</button>
			{#if expanded}
				<span class="panel-title">Groups</span>
				<button class="btn btn-sm" onclick={() => handleToggleAll(true)} title="Show all">
					All
				</button>
				<button class="btn btn-sm" onclick={() => handleToggleAll(false)} title="Hide all">
					None
				</button>
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
{/if}

<style>
	.filter-panel {
		position: fixed;
		top: 16px;
		left: 16px;
		width: 300px;
	}

	.filter-panel.collapsed {
		width: auto;
	}

	.filter-panel.collapsed .panel-header {
		border-bottom: none;
	}

	.toggle-btn {
		margin-left: -8px;
	}

	.panel-content {
		padding: 6px 0;
		max-height: calc(100vh - 8rem);
		overflow-y: auto;
	}

	.group-item {
		display: flex;
		align-items: center;
		gap: 10px;
		padding: 6px 14px;
		cursor: pointer;
	}

	.group-item:hover {
		background: var(--surface-raised);
	}

	.group-item input[type='checkbox'] {
		appearance: none;
		-webkit-appearance: none;
		display: grid;
		place-content: center;
		width: 14px;
		height: 14px;
		flex-shrink: 0;
		border: 1.5px solid var(--ink);
		background: transparent;
		cursor: pointer;
	}

	.group-item input[type='checkbox']::after {
		content: '';
		width: 7px;
		height: 4px;
		border-left: 2px solid var(--on-red);
		border-bottom: 2px solid var(--on-red);
		transform: translate(0, -1px) rotate(-45deg);
		opacity: 0;
	}

	.group-item input[type='checkbox']:checked {
		background: var(--red);
		border-color: var(--red);
	}

	.group-item input[type='checkbox']:checked::after {
		opacity: 1;
	}

	.group-color {
		width: 10px;
		height: 10px;
		border-radius: 50%;
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
		font-size: 12px;
		color: var(--ink-muted);
		font-variant-numeric: tabular-nums;
	}
</style>
