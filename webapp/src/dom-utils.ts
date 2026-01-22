export const getDOMElementById = <T extends HTMLElement>(id: string): T => {
  const element = document.getElementById(id)
  if (!element) {
    throw new Error(`Failed to find DOM element ${id}`)
  }

  return element as T
}

export const makeNode = (
  nodeType: string,
  props: Partial<Record<keyof HTMLElement, string>> & Record<string, unknown>,
  ...children: (HTMLElement | string)[]
) => {
  const element = document.createElement(nodeType)

  Object.entries(props).forEach(([propName, propValue]) => {
    ;(element as unknown as Record<string, unknown>)[propName] = propValue
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

export const removeAllChildren = (node: HTMLElement) => {
  while (node.lastChild) {
    node.removeChild(node.lastChild)
  }
}

export const removeClassFromAllElements = (className: string) => {
  for (const ele of document.getElementsByClassName(className)) {
    ele.classList.remove(className)
  }
}
