<module>
	<service><?=$GETCFG_SVC?></service>
	<device>
		<samba>
			<enable><? echo query("/device/samba/enable");?></enable>
		</samba>
	</device>
</module>
