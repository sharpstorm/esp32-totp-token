export const getDOMElementById = <T extends HTMLElement>(id: string): T => {
  const element = document.getElementById(id)
  if (!element) {
    throw new Error(`Failed to find DOM element ${id}`)
  }

  return element as T
}

export const makeNode = (
  nodeType: string,
  props: Partial<Record<keyof HTMLElement, string>>,
  ...children: (HTMLElement | string)[]
) => {
  const element = document.createElement(nodeType)

  Object.entries(props).forEach(([propName, propValue]) => {
    ;(element as unknown as Record<string, string>)[propName] = propValue
  })

  for (const child of children) {
    if (typeof child === 'string') {
      const span = document.createElement('span')
      span.textContent = child
      element.appendChild(span)
    } else {
      element.appendChild(child)
    }
  }

  return element
}
