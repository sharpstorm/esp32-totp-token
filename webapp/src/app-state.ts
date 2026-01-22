export type ContentTabId = 'wifiConfig' | 'secretConfig' | 'timeSync'
export type AppContentContainerState = {
  activeTab: ContentTabId
}

export const appContentContainerState: AppContentContainerState = {
  activeTab: 'wifiConfig',
}
