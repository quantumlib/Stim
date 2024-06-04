/**
 * @param {!Iterable<TItem> | !Iterator<TItem>}items
 * @param {!function(item: TItem): TKey} func
 * @returns {!Map<TKey, !Array<TItem>>}
 * @template TItem
 * @template TKey
 */
function groupBy(items, func) {
    let result = new Map();
    for (let item of items) {
        let key = func(item);
        let group = result.get(key);
        if (group === undefined) {
            result.set(key, [item]);
        } else {
            group.push(item);
        }
    }
    return result;
}

export {groupBy};
