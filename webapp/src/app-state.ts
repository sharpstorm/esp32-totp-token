export type ContentTabId = 'wifiConfig' | 'secretConfig'
export type AppContentContainerState = {
  activeTab: ContentTabId
}

export const appContentContainerState: AppContentContainerState = {
  activeTab: 'wifiConfig',
}
